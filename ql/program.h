/**
 * @file   program.h
 * @date   04/2017
 * @author Nader Khammassi
 *         Imran Ashraf
 * @brief  openql program
 */

#ifndef QL_PROGRAM_H
#define QL_PROGRAM_H

#include <ql/utils.h>
#include <ql/options.h>
#include <ql/platform.h>
#include <ql/kernel.h>
#include <ql/interactionMatrix.h>
#include <ql/eqasm_compiler.h>
#include <ql/arch/cbox_eqasm_compiler.h>
#include <ql/arch/cc_light_eqasm_compiler.h>
#include <ql/arch/quantumsim_eqasm_compiler.h>

namespace ql
{

extern bool initialized;

/**
 * quantum_program_
 */
class quantum_program
{

   public: 

      std::string           eqasm_compiler_name;
      ql::eqasm_compiler *  backend_compiler;
      ql::quantum_platform  platform; 

   public:

      quantum_program(std::string name, size_t nqubits /**/, quantum_platform platform) : platform(platform), name(name), default_config(true), qubits(nqubits)
      {
         eqasm_compiler_name = platform.eqasm_compiler_name;
	      backend_compiler    = NULL;
         if (eqasm_compiler_name =="")
         {
            EOUT("eqasm compiler name must be specified in the hardware configuration file !");
            throw std::exception();
         }
         else if (eqasm_compiler_name == "none")
         {

         }
         else if (eqasm_compiler_name == "qx")
         {
            // at the moment no qx specific thing is done
         }
         else if (eqasm_compiler_name == "qumis_compiler")
         {
            backend_compiler = new ql::arch::cbox_eqasm_compiler();
         }
	      else if (eqasm_compiler_name == "cc_light_compiler" )
	      {
            backend_compiler = new ql::arch::cc_light_eqasm_compiler();
	      }
         else if (eqasm_compiler_name == "quantumsim_compiler" )
         {
            backend_compiler = new ql::arch::quantumsim_eqasm_compiler();
         }
         else
         {
            EOUT("the '" << eqasm_compiler_name << "' eqasm compiler backend is not suported !");
            throw std::exception();
         }

         if(nqubits > platform.qubit_number)
         {
            EOUT("number of qubits requested in program '" + std::to_string(nqubits) + "' is greater than the qubits available in platform '" + std::to_string(platform.qubit_number) + "'" );
            throw ql::exception("[x] error : number of qubits requested in program '"+std::to_string(nqubits)+"' is greater than the qubits available in platform '"+std::to_string(platform.qubit_number)+"' !",false);
         }
      }

      void add(ql::quantum_kernel k)
      {
         // check sanity of supplied qubit numbers for each gate
         ql::circuit& kc = k.get_circuit();
         for( auto & g : kc )
         {
            auto & gate_operands = g->operands;
            auto & gname = g->name;
            for(auto & qno : gate_operands)
            {
               // DOUT("qno : " << qno);
               if( qno >= qubits )
               {   
                   EOUT("No of qubits in program: " << qubits << ", specified qubit number out of range for gate:'" << gname << "' with " << ql::utils::to_string(gate_operands,"qubits") );
                   throw ql::exception("[x] error : ql::kernel::gate() : No of qubits in program: "+std::to_string(qubits)+", specified qubit number out of range for gate '"+gname+"' with " +ql::utils::to_string(gate_operands,"qubits")+" !",false);
               }
            }
         }

         // if sane, now add kernel to list of kernels
         kernels.push_back(k);
      }

      void set_config_file(std::string file_name)
      {
         config_file_name = file_name;
         default_config   = false;
      }

      std::string qasm()
      {
         std::stringstream ss;
         ss << "# this file has been automatically generated by the OpenQL compiler please do not modify it manually.\n";
         ss << "qubits " << qubits << "\n";
         for (size_t k=0; k<kernels.size(); ++k)
            ss <<'\n' << kernels[k].qasm();
	/*
         ss << ".cal0_1\n";
         ss << "   prepz q0\n";
         ss << "   measure q0\n";
         ss << ".cal0_2\n";
         ss << "   prepz q0\n";
         ss << "   measure q0\n";
         ss << ".cal1_1\n";
         ss << "   prepz q0\n";
         ss << "   x q0\n";
         ss << "   measure q0\n";
         ss << ".cal1_2\n";
         ss << "   prepz q0\n";
         ss << "   x q0\n";
         ss << "   measure q0\n";
	 */
         return ss.str();
      }

      std::string microcode()
      {
         std::stringstream ss;
         ss << "# this file has been automatically generated by the OpenQL compiler please do not modify it manually.\n";
         ss << uc_header();
         for (size_t k=0; k<kernels.size(); ++k)
            ss <<'\n' << kernels[k].micro_code();
	 /*
         ss << "     # calibration points :\n";  // wait for 100us
         // calibration points for |0>
         for (size_t i=0; i<2; ++i)
         {
            ss << "     waitreg r0               # prepz q0 (+100us) \n";  // wait for 100us
            ss << "     waitreg r0               # prepz q0 (+100us) \n";  // wait for 100us
            // measure
            ss << "     wait 60 \n";  // wait
            ss << "     pulse 0000 1111 1111 \n";  // pulse
            ss << "     wait 50\n";
            ss << "     measure\n";  // measurement discrimination
         }

         // calibration points for |1>
         for (size_t i=0; i<2; ++i)
         {
            // prepz
            ss << "     waitreg r0               # prepz q0 (+100us) \n";  // wait for 100us
            ss << "     waitreg r0               # prepz q0 (+100us) \n";  // wait for 100us
            // X
            ss << "     pulse 1001 0000 1001     # X180 \n";
            ss << "     wait 10\n";
            // measure
            ss << "     wait 60\n";  // wait
            ss << "     pulse 0000 1111 1111\n";  // pulse
            ss << "     wait 50\n";
            ss << "     measure\n";  // measurement discrimination
         }
	 */

         ss << "     beq  r3,  r3, loop   # infinite loop";
         return ss.str();
      }

      void set_platform(quantum_platform platform)
      {
         this->platform = platform;
      }

      std::string uc_header()
      {
         std::stringstream ss;
         ss << "# auto-generated micro code from rb.qasm by OpenQL driver, please don't modify it manually \n";
         ss << "mov r11, 0       # counter\n";
         ss << "mov r3,  10      # max iterations\n";
         ss << "mov r0,  20000   # relaxation time / 2\n";
         // ss << name << ":\n";
         ss << "loop:\n";
         return ss.str();
      }

      int compile()
      {
         IOUT("compiling ...");

         if (kernels.empty())
            return -1;

         if( ql::options::get("optimize") == "yes" )
         {
            IOUT("optimizing quantum kernels...");
            for (size_t k=0; k<kernels.size(); ++k)
               kernels[k].optimize();
         }


         auto tdopt = ql::options::get("decompose_toffoli");
         if( tdopt == "AM" || tdopt == "NC" )
         {
            IOUT("Decomposing Toffoli ...");
            for (size_t k=0; k<kernels.size(); ++k)
               kernels[k].decompose_toffoli();
         }
         else if( tdopt == "no" )
         {
            IOUT("Not Decomposing Toffoli ...");
         }
         else
         {
            EOUT("Unknown option '" << tdopt << "' set for decompose_toffoli");
            throw ql::exception("Error: Unknown option '"+tdopt+"' set for decompose_toffoli !",false);
         }


         map();

         schedule();

         if (backend_compiler == NULL)
         {
            WOUT("no eqasm compiler has been specified in the configuration file, only qasm code has been compiled.");
            return 0;
         }


         IOUT("fusing quantum kernels...");
         ql::circuit fused;
         for (size_t k=0; k<kernels.size(); ++k)
         {
            ql::circuit& kc = kernels[k].get_circuit();
            for(size_t i=0; i<kernels[k].iterations; i++)
            {
               fused.insert(fused.end(),kc.begin(),kc.end());   
            }
         }

      	try 
      	{
      	   IOUT("compiling eqasm code...");
      	   backend_compiler->compile(name, fused, platform);
      	}
      	catch (ql::exception e)
      	{
      	   EOUT("[x] error : eqasm_compiler.compile() : compilation interrupted due to fatal error.");
      	   throw e;
      	}

         IOUT("writing eqasm code to '" << ( ql::options::get("output_dir") + "/" + name+".asm"));
         backend_compiler->write_eqasm( ql::options::get("output_dir") + "/" + name + ".asm");

         IOUT("writing traces to '" << ( ql::options::get("output_dir") + "/trace.dat"));
         backend_compiler->write_traces( ql::options::get("output_dir") + "/trace.dat");

         // deprecated hardcoded microcode generation 

         /*
         std::stringstream ss_asm;
         ss_asm << output_path << name << ".asm";
         std::string uc = microcode();

         IOUT("writing transmon micro-code to '" << ss_asm.str() << "' ...");
         ql::utils::write_file(ss_asm.str(),uc);
         */

         if (sweep_points.size())
         {
            std::stringstream ss_swpts;
            ss_swpts << "{ \"measurement_points\" : [";
            for (size_t i=0; i<sweep_points.size()-1; i++)
               ss_swpts << sweep_points[i] << ", ";
            ss_swpts << sweep_points[sweep_points.size()-1] << "] }";
            std::string config = ss_swpts.str();
            if (default_config)
            {
               std::stringstream ss_config;
               ss_config << ql::options::get("output_dir") << "/" << name << "_config.json";
               std::string conf_file_name = ss_config.str();
               IOUT("writing sweep points to '" << conf_file_name << "'...");
               ql::utils::write_file(conf_file_name, config);
            }
            else
            {
               std::stringstream ss_config;
               ss_config << ql::options::get("output_dir") << "/" << config_file_name;
               std::string conf_file_name = ss_config.str();
               IOUT("writing sweep points to '" << conf_file_name << "'...");
               ql::utils::write_file(conf_file_name, config);
            }
         }
         else
         {
            EOUT("cannot write sweepoint file : sweep point array is empty !");
         }

	      IOUT("compilation of program '" << name << "' done.");

         return 0;
      }

      void map()
      {
          auto mapopt = ql::options::get("mapper");
          // DOUT("mapping option=" << mapopt);
          if (mapopt == "base")
          {
             for (auto k : kernels)
             {
                k.map(qubits, platform);
             }
             for (auto k : kernels)
             {
                DOUT("Qasm at end of program::map size=" << k.c.size() << ":");
                DOUT(k.qasm());
                DOUT("Qasm at end of program::map END");
             }
          }
          else if (mapopt == "no" )
          {
             IOUT("Not mapping the quantum program");
          }
          else
          {
             EOUT("Unknown option '" << mapopt << "' set for mapper");
             throw ql::exception("Error: Unknown option '"+mapopt+"' set for mapper !",false);
          }
      }

      void schedule()
      {
         std::string sched_qasm;

         IOUT("scheduling the quantum program");

         sched_qasm = "qubits " + std::to_string(qubits) + "\n";
         for (auto k : kernels)
         {
            std::string kernel_sched_qasm;
            std::string kernel_sched_dot;

	    DOUT("Qasm at start of program::schedule size=" << k.c.size() << ":");
	    DOUT(k.qasm());
	    DOUT("Qasm at start of program::schedule END");

            k.schedule(qubits, platform, kernel_sched_qasm, kernel_sched_dot);
            if( k.iterations > 1 )
               sched_qasm += "\n." + k.get_name() + "("+ std::to_string(k.iterations) + ")";
            else
               sched_qasm += "\n." + k.get_name();
            sched_qasm += kernel_sched_qasm + '\n';
            // disabled generation of dot file for each kernel
            // string fname = ql::options::get("output_dir") + "/" + k.get_name() + scheduler + ".dot";
            // IOUT("writing scheduled qasm to '" << fname << "' ...");
            // ql::utils::write_file(fname, kernel_sched_dot);
         }

         string fname = ql::options::get("output_dir") + "/" + name + "_scheduled.qasm";
         IOUT("writing scheduled qasm to '" << fname << "' ...");
         ql::utils::write_file(fname, sched_qasm);
      }

      void print_interaction_matrix()
      {
         IOUT("printing interaction matrix...");

         for (auto k : kernels)
         {
            InteractionMatrix imat( k.get_circuit(), qubits);
            string mstr = imat.getString();
            std::cout << mstr << std::endl;
         }
      }

      void write_interaction_matrix()
      {
         for (auto k : kernels)
         {
            InteractionMatrix imat( k.get_circuit(), qubits);
            string mstr = imat.getString();

            string fname = ql::options::get("output_dir") + "/" + k.get_name() + "InteractionMatrix.dat";
            IOUT("writing interaction matrix to '" << fname << "' ...");
            ql::utils::write_file(fname, mstr);
         }
      }

      void set_sweep_points(float * swpts, size_t size)
      {
         sweep_points.clear();
         for (size_t i=0; i<size; ++i)
            sweep_points.push_back(swpts[i]);
      }

   protected:

      // ql_platform_t               platform;
      std::vector<quantum_kernel> kernels;
      std::vector<float>          sweep_points;
      std::string                 name;
      // std::string                 output_path;
      std::string                 config_file_name;
      bool                        default_config;
      size_t                      qubits;
};

} // ql

#endif //QL_PROGRAM_H
