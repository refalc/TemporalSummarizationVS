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
	//omp_set_num_threads(1);
	CLogger::Instance()->WriteToLog("Start perf check");
	auto probe = CProfiler::CProfilerProbe("all");
	CArgReader arg_reader;
	bool allow_test_args = true;
	//__debugbreak();
	if( !arg_reader.ReadArguments(argc, argv) ) {
		if( !allow_test_args )
			return -1;

		std::vector<std::string> test_args = 
		{ "C:\\!DEV\\C++\\TemporalSummarization\\TemporalSummarizationVS\\x64\\Release\\TemporalSummarization.exe",
			"5", "9926682","11092876", "10371644", "10419394", "11092876",
			"-a", "C:\\!DEV\\C++\\TemporalSummarization\\TemporalSummarizationVS\\Data\\answer.xml",
			"-c", "C:\\!DEV\\C++\\Diplom\\TemporalSummarization\\TemporalSummarization\\start_config.xml",
			"-e", "C:\\Users\\MishaDEV\\Data\\news_corp_tr_last_w5_s100_c10.bin",
			"-d", "C:\\!DEV\\C++\\TemporalSummarization\\TemporalSummarizationVS\\Data\\Docs\\"
		};
		if( !arg_reader.ReadArguments((int)test_args.size(), test_args.data()) )
			return -1;
	}

	TSController controller;
	Params input_params;
	std::string input_answer_path, input_w2v_path, docs_serialization_path, topic_modeling_path;
	std::vector<std::string> input_queries;

	int summary_size = 15;
	if( !arg_reader.GetParams(input_params) || !arg_reader.GetAnswerPath(input_answer_path) ||
		!arg_reader.GetW2VPath(input_w2v_path) || !arg_reader.GetQueries(input_queries) ||
		!arg_reader.GetDocsSerializationPath(docs_serialization_path) || !arg_reader.GetTopicModelingPath(topic_modeling_path)) {
		CLogger::Instance()->WriteToLog("ERROR : while getting parameters from arg reader");
		return -1;
	}

	if( !controller.InitParameters(input_params, input_answer_path, docs_serialization_path, summary_size, input_w2v_path, topic_modeling_path) ) {
		CLogger::Instance()->WriteToLog("ERROR : while init parameters");
		return -1;
	}

	if( !controller.RunQueries(input_queries) ) {
		CLogger::Instance()->WriteToLog("ERROR : while RunQueries");
		return -1;
	}

	
	CLogger::Instance()->WriteToLog("End perf check");
    return 0;
}



