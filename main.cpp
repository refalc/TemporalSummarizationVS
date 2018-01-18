#include <iostream>
#include "TemporalSummarization\TSDataExtractor.h"
#include "TemporalSummarization\utils.h"
#include "TemporalSummarization\TSQueryConstructor.h"
#include "TemporalSummarization\TSDocumentExtractor.h"
#include "TemporalSummarization\TSSolver.h"
#include "TemporalSummarization\TSController.h"
#include "TemporalSummarization\utils.h"

int main(int argc, char *argv[])
{
	CLogger::Instance()->WriteToLog("Start perf check");
	/*
	TSDataExtractor data_extractor;
	TSDocCollection collection;
	TSDocumentPtr doc_ptr = collection.AllocateDocument();
	data_extractor.GetDocument("13197524", doc_ptr);

	TSSentenceConstPtr sentence =  &(*doc_ptr->sentences_begin());
	TSIndexConstPtr p_index;
	sentence->GetIndex(SDataType::LEMMA, p_index);

	Word2Vec model;
	model.Load("C:\\Users\\MishaDEV\\Data\\news_corp_tr_last_w5_s100_c10.bin");

	p_index->ConstructIndexEmbedding(&model);

	std::cout << sentence->EmbeddingSimilarity(*sentence, SDataType::LEMMA) << std::endl;
	*/
	auto probe = CProfiler::CProfilerProbe("all");

	CArgReader arg_reader;
	if( arg_reader.ReadArguments(argc, argv) ) {
		TSController controller;
		Params input_params;
		std::string input_answer_path, input_w2v_path;
		std::vector<std::string> input_queries;

		int summary_size = 15;
		if( !arg_reader.GetParams(input_params) || !arg_reader.GetAnswerPath(input_answer_path) || !arg_reader.GetW2VPath(input_w2v_path) || !arg_reader.GetQueries(input_queries) ) {
			CLogger::Instance()->WriteToLog("ERROR : while getting parameters from arg reader");
			return -1;
		}

		if( !controller.InitParameters(input_params, input_answer_path, input_w2v_path, summary_size) ) {
			CLogger::Instance()->WriteToLog("ERROR : while init parameters");
			return -1;
		}

		if( !controller.RunQueries(input_queries) ) {
			CLogger::Instance()->WriteToLog("ERROR : while RunQueries");
			return -1;
		}
	} else {
		std::vector<std::string> test_args = { "C:\\!DEV\\C++\\TemporalSummarization\\TemporalSummarizationVS\\x64\\Release\\TemporalSummarization.exe", "45", "12263137", "11092876", "11872175", "10825393", "12171411", "13197524", "10482437", "10365689", "12782702", "11155970", "12521721", "11768974", "10781778", "11770254", "10839989", "10167143", "10466159", "12082485", "11870621", "12884293", "11086272", "10482258", "10822682", "13394061", "13324368", "10138558", "11136413", "12209942", "11155739", "12483331", "12156694", "13322307", "11134760", "13055100", "13142685", "13377372", "10105996", "10366234", "10372602", "11772232", "12367599", "12458844", "12154084", "10824477", "9926682", "-a", "C:\\!DEV\\C++\\TemporalSummarization\\TemporalSummarizationVS\\Data\\answer.xml", "-c", "C:\\!DEV\\C++\\Diplom\\TemporalSummarization\\TemporalSummarization\\start_config.xml", "-e", "C:\\Users\\MishaDEV\\Data\\news_corp_tr_last_w5_s100_c10.bin" };
		//std::vector<std::string> test_args = { "C:\\!DEV\\C++\\TemporalSummarization\\TemporalSummarizationVS\\x64\\Release\\TemporalSummarization.exe", "10", "11092876","11092876","11092876","11092876","11092876","11092876","11092876","11092876","11092876","11092876", "-a", "C:\\!DEV\\C++\\TemporalSummarization\\TemporalSummarizationVS\\Data\\answer.xml", "-c", "C:\\!DEV\\C++\\Diplom\\TemporalSummarization\\TemporalSummarization\\start_config.xml", "-e", "C:\\Users\\MishaDEV\\Data\\news_corp_tr_last_w5_s100_c10.bin" };

		if( arg_reader.ReadArguments((int)test_args.size(), test_args.data()) ) {
			TSController controller;
			Params input_params;
			std::string input_answer_path, input_w2v_path;
			std::vector<std::string> input_queries;

			int summary_size = 15;
			if( !arg_reader.GetParams(input_params) || !arg_reader.GetAnswerPath(input_answer_path) || !arg_reader.GetW2VPath(input_w2v_path) || !arg_reader.GetQueries(input_queries) ) {
				CLogger::Instance()->WriteToLog("ERROR : while getting parameters from arg reader");
				return -1;
			}

			if( !controller.InitParameters(input_params, input_answer_path, input_w2v_path, summary_size) ) {
				CLogger::Instance()->WriteToLog("ERROR : while init parameters");
				return -1;
			}

			if( !controller.RunQueries(input_queries) ) {
				CLogger::Instance()->WriteToLog("ERROR : while RunQueries");
				return -1;
			}
		} else {
			TSController controller;
			Params input_params;

			input_params.m_MaxAnswerSize = 15;            //todo task_param
			input_params.m_PMinSentSize = 4;              //todo hardcode
			input_params.m_PMinMMR = 0.0001;              //todo hardcode
			input_params.m_PowerMethodEps = 0.0000001;    //todo hardcode
			input_params.m_SentSimThreshold = 0.8;

			input_params.m_PDocCount = 500;
			input_params.m_QEQuerrySize = 10;
			input_params.m_QETopLemms = 15;
			input_params.m_QETopTermins = 3;
			input_params.m_QEDocCount = 50;
			input_params.m_PSoftOr = 0.3;
			input_params.m_DIMinLinkScore = 0.65;
			input_params.m_PKeepL = 5;
			input_params.m_PKeepT = 2;
			input_params.m_PLambda = 0.84;
			input_params.m_QEMinDocRank = 0.3;
			input_params.m_PTemporalMode = false;
			input_params.m_DocImportance = false;
			input_params.m_DIAlpha = 0.35;
			input_params.m_DIPowerMethodDFactor = 0.35;
			input_params.m_TempMaxDailyAnswerSize = 15;
			input_params.m_DIDocBoundary = 0.7;
			input_params.m_PQuerryEx = true;
			input_params.m_QEDEInitQuerrySize = 5;
			input_params.m_QEDoubleExtension = true;

			std::string input_answer_path = "..//Data//answer.xml", input_w2v_path = "../w2v.bin";
			std::vector<std::string> input_queries = { "10839989" };

			int summary_size = input_params.m_MaxAnswerSize;
			if( !controller.InitParameters(input_params, input_answer_path, input_w2v_path, summary_size) )
				return -1;

			if( !controller.RunQueries(input_queries) )
				return -1;
		}
	}
	
	CLogger::Instance()->WriteToLog("End perf check");
    return 0;
}



