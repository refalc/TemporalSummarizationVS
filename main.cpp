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
	auto t1_s = std::chrono::high_resolution_clock::now();

	CArgReader arg_reader;
	if( arg_reader.ReadArguments(argc, argv) ) {
		TSController controller;
		Params input_params;
		std::string input_answer_path, input_w2v_path;
		std::vector<std::string> input_queries;

		int summary_size = 15;
		if( !arg_reader.GetParams(input_params) || !arg_reader.GetAnswerPath(input_answer_path) || !arg_reader.GetW2VPath(input_w2v_path) )
			return -1;

		if( !controller.InitParameters(input_params, input_answer_path, input_w2v_path, summary_size) )
			return -1;

		if( !controller.RunQueries(input_queries) )
			return -1;
	} else {
		TSController controller;
		Params input_params;

		input_params.m_MaxAnswerSize = 15;            //todo task_param
		input_params.m_PMinSentSize = 4;              //todo hardcode
		input_params.m_PMinMMR = 0.0001;              //todo hardcode
		input_params.m_PMaxSentSize = 50;             //todo hardcode
		input_params.m_PowerMethodEps = 0.0000001;    //todo hardcode
		input_params.m_SentSimThreshold = 0.8;

		input_params.m_PDocCount = 500;
		input_params.m_QEQuerrySize = 10;
		input_params.m_QETopLemms = 15;
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
		std::vector<std::string> input_queries = {"10365689"};

		int summary_size = input_params.m_MaxAnswerSize;
		if( !controller.InitParameters(input_params, input_answer_path, input_w2v_path, summary_size) )
			return -1;

		if( !controller.RunQueries(input_queries) )
			return -1;
	}
	/*TSDataExtractor data_extractor;
	TSQueryConstructor query_constructor;
	if( !query_constructor.InitParameters({ 5, 2, 50, 0.5f, 0.0f, 15, 5, 15, 3 }) || !query_constructor.InitDataExtractor(&data_extractor) )
		return 0;

	TSQuery query;
	std::string doc_id = "10365689";
	if( !query_constructor.QueryConstructionProcess(doc_id, query) )
		return 0;

	TSQuery extended_query;
	if( !query_constructor.QueryExtensionProcess(query, extended_query) )
		return 0;

	TSQuery double_extended_query;
	if( !query_constructor.QueryExtensionProcess(extended_query, double_extended_query) )
		return 0;

	TSIndexConstPtr lemma_index;
	query.GetIndex(SDataType::LEMMA, lemma_index);
	CLogger::Instance()->WriteToLog(lemma_index->GetString());
	extended_query.GetIndex(SDataType::LEMMA, lemma_index);
	CLogger::Instance()->WriteToLog(lemma_index->GetString());
	double_extended_query.GetIndex(SDataType::LEMMA, lemma_index);
	CLogger::Instance()->WriteToLog(lemma_index->GetString());

	TSDocumentExtractor doc_extractor;
	if( !doc_extractor.InitParameters({ 500, 0.3f, 0.0f }) || !doc_extractor.InitDataExtractor(&data_extractor) )
		return 0;

	TSTimeLineCollections collections;
	if( !doc_extractor.ConstructTimeLineCollections(double_extended_query, collections) )
		return false;


	TSTimeLineQueries queries;
	queries.AddQuery(0, std::move(double_extended_query));

	TSSolver solver;
	std::vector<std::pair<float, TSSentenceConstPtr>> temporal_summary;
	solver.GetTemporalSummary(collections, queries, 15, temporal_summary);
	*/
	auto t1_e = std::chrono::high_resolution_clock::now();
	CProfiler::Instance()->AddDuration("all", (double)std::chrono::duration_cast<std::chrono::microseconds>(t1_e - t1_s).count() / 1e6);
	CProfiler::Instance()->DataToLog();

	CLogger::Instance()->WriteToLog("End perf check");
    return 0;
}



