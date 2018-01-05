#include <iostream>
#include "TemporalSummarization\TSDataExtractor.h"
#include <fstream>
#include "TemporalSummarization\utils.h"
#include "TemporalSummarization\TSQueryConstructor.h"
#include "TemporalSummarization\TSDocumentExtractor.h"
#include "TemporalSummarization\TSSolver.h"

int main(int argc, char *argv[])
{
	//std::locale::global(std::locale("Russian"));
	CLogger::Instance()->WriteToLog("Start perf check");
	auto t1_s = std::chrono::high_resolution_clock::now();
	TSDataExtractor data_extractor;
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

	auto t1_e = std::chrono::high_resolution_clock::now();
	CProfiler::Instance()->AddDuration("all", (double)std::chrono::duration_cast<std::chrono::microseconds>(t1_e - t1_s).count() / 1e6);
	CProfiler::Instance()->DataToLog();

	CLogger::Instance()->WriteToLog("End perf check");
    return 0;
}



