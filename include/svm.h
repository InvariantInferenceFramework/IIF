#ifndef _SVM_H_
#define _SVM_H_
#include "ml_algo.h"
#include "svm_core.h"
#include "string.h"




typedef double MState[D4mapping];


class SVM : public MLalgo
{
	private:
		svm_parameter param;
		svm_problem problem;
		svm_model* pre_model;
		Equation* classifier;
		int max_size;

		double* label; // [max_items * 2];
		double** data; // [max_items * 2];
		//double** pdata; // [max_items * 2];
		MState* pdata;

		int mapping_type;
		int mapping_dimension;

	public:
		svm_model* model;

	protected:
		int enlargeSize(int new_size) {
			assert (new_size >= max_size);
			int valid_size = problem.l;

			// enlarge max_size exponentially to cover all the data.
			while (new_size >= max_size) max_size *= 2;

			double** pre_data = data;
			data = new double*[max_size];
			memmove(data, pre_data, valid_size * sizeof(double**));
			delete []pre_data;

			double* pre_label = label;
			label = new double[max_size];
			memmove(label, pre_label, valid_size * sizeof(double*));
			delete pre_label;
			return 0;
		}


	public:
		inline Equation* getClassifier() {
			return classifier;
		}

		inline svm_model* getModel() {
			return model;
		}

		inline int setClassifier(Equation* e) {
			*classifier = *e;
			return 0;
		}

		SVM(int type = 0, void (*f) (const char*) = NULL, int size = 1000000) : max_size(size) {
			prepare_svm_parameters(param, type);
			if (f != NULL)
				svm_set_print_string_function(f);
			model = NULL;
			classifier = NULL;

			data = new double*[max_size];
			pdata = new MState[max_size];
			label = new double[max_size];
			for (int i = 0; i < max_size; i++)
				label[i] = -1;
			problem.l = 0;
			problem.x = (svm_node**)(data);
			problem.y = label;
			model = NULL;
			pre_model = NULL;
			//pdata = NULL;
			mapping_type = 0;
			mapping_dimension = 0;
		}

		~SVM() {
			//if (model != NULL) delete model;
			if (model != NULL) svm_free_and_destroy_model(&model);
			if (pre_model != NULL) svm_free_and_destroy_model(&pre_model);
			if (classifier != NULL) delete classifier;
			if (data != NULL) delete []data;
			if (pdata != NULL) delete[]pdata;
			if (label != NULL) delete label;
		}

		void setDegree(int degree) {
			if (degree <= 0) return;
			param.degree = degree;
		}


		int makeTrainingSet(States* gsets, int& pre_psize, int& pre_nsize) {
			int cur_psize = gsets[POSITIVE].getSize();
			int cur_nsize = gsets[NEGATIVE].getSize();
			if (cur_psize + cur_nsize >= max_size) 
				enlargeSize(cur_psize + cur_nsize);
			
#ifdef __PRT
			std::cout << "+[" << cur_psize - pre_psize << "|"
				<< cur_nsize - pre_nsize  << "] ==> ["
				<< cur_psize << "+|" << cur_nsize << "-]";
#endif

			//POINTER
			// prepare new training data set
			// training set & label layout:
			// data :  0 | positive states | negative states | ...
			// label:    | 1, 1, ..., 1, . | -1, -1, ..., -1, -1, -1, ...
			// move the negative states from old OFFSET: [pre_positive_size] to new OFFSET: [cur_positive_size]
			memmove(data + cur_psize, data + pre_psize, pre_nsize * sizeof(double*));

			// add new positive states at OFFSET: [pre_positive_size]
			int cur_index = pre_psize + pre_nsize;
			for (int i = 0 ; i < cur_psize - pre_psize; i++) {
				mappingData(gsets[POSITIVE].values[i], pdata[cur_index + i], 4);
				data[pre_psize + i] = pdata[cur_index + i];
				label[pre_psize + i] = 1;
			}
			/*
			for (int i = pre_psize; i < cur_psize; i++) {
				data[i] = gsets[POSITIVE].values[i];
				label[i] = 1;
			}
			*/
			// add new negative states at OFFSET: [cur_positive_size + pre_negative_size]
			cur_index = cur_psize + pre_nsize;
			for (int i = 0 ; i < cur_nsize - pre_nsize; i++) {
				mappingData(gsets[NEGATIVE].values[i], pdata[cur_index + i], 4);
				data[cur_index + i] = pdata[cur_index + i];
				label[cur_index + i] = -1;
			}
			/*
			for (int i = pre_nsize; i < cur_nsize; i++) {
				//data[cur_psize + i] = gsets[NEGATIVE].values[i];
				data[cur_psize + i] = gsets[NEGATIVE].values[i];
			}
			*/
			problem.l = cur_psize + cur_nsize;
			int ret = cur_psize + cur_nsize - pre_psize - pre_nsize;
			pre_psize = cur_psize;
			pre_nsize = cur_nsize;
			//mappingDataSet();
			std::cout << BLUE << problem << WHITE << std::endl;
			return ret;
		}

		int typeChanger(int type) {
			if ((type < 1) || (type > 4))
				return -1;
			switch (type) {
				case 1:
					return setDimension(D1mapping);
				case 2:
					return setDimension(D2mapping);
				case 3:
					return setDimension(D3mapping);
				case 4:
					return setDimension(D4mapping);
			}
			return -1;
			//return setDimension(D##type##mapping);
		}

		void setMapping(int type) {
			std::cout << "--->>> set mapping " << type << "  ";
			if ((type < 1) || (type > 4))
				std::cout << "warning: mapping type is out of bounds.\n";
			mapping_type = type;
		}

		bool mappingDataSet(){
			/*
			std::cout << "--->>> mapping data set.\n";
			if (mapping_type < 1) return false;
			if (mapping_type > 4) return false;
			if (mapping_type == 1) {
				problem.x = (svm_node**)data;
				return true;
			}
			mapping_dimension = 0;
			switch (mapping_type) {
				case 4:
					mapping_dimension += VARS * (VARS + 1) / 2 * VARS * (VARS + 1) / 2; 
				case 3:
					mapping_dimension += VARS * (VARS + 1) * (2 * VARS + 1) / 6; 
				case 2:
					mapping_dimension += VARS * (VARS + 1) / 2;
				case 1:
					mapping_dimension += VARS;
				default:
					break;
			}
			std::cout << "mapping dimension = " << mapping_dimension << std::endl;
				
			for (int i = 0; i < problem.l; i++) {
				//pdata[i] = new double[mapping_dimension];
				//assign value to pdata[i] 
				std::cout << "check point @i=" << i << std::endl;
				if (mappingData(data[i], pdata[i], mapping_type) == false) {
					std::cout << "<<<---F mapping data set.\n";
					return false;
				}
			}
					
			problem.x = (svm_node**)pdata;
			std::cout << "<<<--- mapping data set.\n";
			*/
			return true;
		}

		bool mappingData(double* src, double* dst, int mp_type = 4) {
			int index = 0;
			if (mp_type >= 1) {
				for (int i = 0; i < VARS; i++) {
					dst[index++] = src[i];
				}
			}
			if (mp_type >= 2) {
				for (int i = 0; i < VARS; i++) {
					for (int j = i; j < VARS; j++) {
						dst[index++] = src[i] * src[j];
					}
				}
			}
			if (mp_type >= 3) {
				for (int i = 0; i < VARS; i++) {
					for (int j = i; j < VARS; j++) {
						for (int k = j; k < VARS; k++) {
							dst[index++] = src[i] * src[j] * src[k];
						}
					}
				}
			}
			if (mp_type >= 4) {
				for (int i = 0; i < VARS; i++) {
					for (int j = i; j < VARS; j++) {
						for (int k = j; k < VARS; k++) {
							for (int l = k; l < VARS; l++) {
								dst[index++] = src[i] * src[j] * src[k] * src[l];
							}
						}
					}
				}
			}
			if (mp_type >= 5) {
				std::cout << "Unsupported for 5 dimension up.\n";
				return false;
			}
			return true;
		}

		int train() {
			if (problem.y == NULL || problem.x == NULL) return -1;
			const char* error_msg = svm_check_parameter(&problem, &param);
			if (error_msg) { 
				std::cout << "ERROR: " << error_msg << std::endl; 
				return -1; 
			}

			if (model != NULL) {
				if (pre_model != NULL)
					svm_free_and_destroy_model(&pre_model);
				pre_model = model;
			}
			//std::cout << "checking point 1\n";
			//std::cout << problem << std::endl;
			model = svm_train(&problem, &param);
			//std::cout << "model --> " << *model << std::endl;
			//std::cout << "checking point 2\n";
			/* if (classifier == NULL) classifier = new Equation();
			 svm_model_visualization(model, *classifier);
			*/
			//svm_free_and_destroy_model(&model);
			//model = NULL;
			return 0;
		}

		double checkTrainingSet() {
			if (problem.l <= 0) return 0;
			int pass = 0;
			for (int i = 0; i < problem.l; i++) {
				//pass += (Equation::calc(*classifier, (double*)problem.x[i]) * problem.y[i] > 0) ? 1 : 0;
				pass += (predict((double*)problem.x[i]) * problem.y[i] >= 0) ? 1 : 0;
			}
			return static_cast<double>(pass) / problem.l;
		}

		int checkQuestionSet(States& qset) {
			/*
			if (classifier == NULL) return -1;
#ifdef __PRT
			std::cout << " [" << qset.traces_num() << "]";
#endif
			for (int i = 0; i < qset.p_index; i++) {
				int pre = -1, cur = 0;
#ifdef __PRT
				std::cout << ".";
#endif
				//std::cout << "\t\t" << i << ">";
				//gsets[QUESTION].print_trace(i);
				for (int j = qset.t_index[i]; j < qset.t_index[i + 1]; j++) {
					cur = Equation::calc(*classifier, qset.values[j]);
					//std::cout << ((cur >= 0) ? "+" : "-");
					if ((pre >= 0) && (cur < 0)) {
						// deal with wrong question trace.
						// Trace back to print out the whole trace and the predicted labels.
#ifdef __PRT
						std::cerr << RED << "\t[FAIL]\n \t  Predict wrongly on Question traces." << std::endl;
						qset.dumpTrace(i);
#endif
						for (int j = qset.t_index[i]; j < qset.t_index[i + 1]; j++) {
							cur = Equation::calc(*classifier, qset.values[j]);
#ifdef __PRT
							std::cout << ((cur >= 0) ? "+" : "-");
#endif
						}
#ifdef __PRT
						std::cout << std::endl << WHITE;
#endif
						return -1;
					}
					pre = cur;
				}
				//std::cout << "END" << std::endl;
			}
#ifdef __PRT
			std::cout << " [PASS]";
#endif
*/
			return 0;
		}

		int converged (void* pre_model, int num =1) {
			assert ((num == 1) || "SVM::get_converged: Unexpected equation number parameter.");
			if (pre_model == NULL) return 1;
			Equation* pre_classifier = (Equation*)pre_model;
			return classifier->is_similar(*pre_classifier);
		}

		bool converged_model () {
			return model_converged(model, pre_model);
		}

		friend std::ostream& operator << (std::ostream& out, const SVM& svm) {
			return svm._print(out);
		}

		std::ostream& _print(std::ostream& out) const {
			out << "SVM-model: ";
			out << *model << std::endl;
			//svm_model_visualization(model, *classifier);
			//out << *classifier; // << std::endl;
			return out;
		}

		int getProblemSize() {
			return problem.l;
		}

		Equation* roundoff(int& num) {
			num = 1;
			Equation* equ = new Equation();
			classifier->roundoff(*equ);
			return equ;
		}

		int predict(double* v) {
			if (model == NULL) return -2;
			if (v == NULL) return -2;
			double res = svm_predict(model, (svm_node*)v); 

			if (res >= 0) return 1;
			else return -1;
		}

		/*int predict(double* v) {
			if (classifier == NULL) return -2;
			if (v == NULL) return -2;
			double res = Equation::calc(*classifier, v);

			if (res >= 0) return 1;
			else return -1;
		}
		*/


};

#endif /* _SVM_H */
