#include <iostream>
#include "TemporalSummarization\TSDataExtractor.h"
#include <fstream>
#include "TemporalSummarization\utils.h"
#include "TemporalSummarization\TSQueryConstructor.h"

int main(int argc, char *argv[])
{
	//std::locale::global(std::locale("Russian"));
	CLogger::Instance()->WriteToLog("Start perf check");
	auto t1_s = std::chrono::high_resolution_clock::now();
	TSDataExtractor data_extractor;
	if( data_extractor.InitParameters({ 500, 0.3f, 0.0f }) != ReturnCode::TS_NO_ERROR )
		return 0;

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

	auto t1_e = std::chrono::high_resolution_clock::now();
	CProfiler::Instance()->AddDuration("all", (double)std::chrono::duration_cast<std::chrono::microseconds>(t1_e - t1_s).count() / 1e6);
	CProfiler::Instance()->DataToLog();
	/*for( int i = 0; i < 500; i++ ) {
		TSDocument doc;
		data_extractor.GetDocument(doc, "11633162");
	}*/
	/*std::vector<TSIndex> query;
	query.push_back(TSIndex(SDataType::LEMMA));
	query.push_back(TSIndex(SDataType::TERMIN));

	query[0].AddToIndex(TSIndexItem("ïàðèæ", 1));
	query[0].AddToIndex(TSIndexItem("òåðàêò", 1));
	query[0].AddToIndex(TSIndexItem("êàðèêàòóðà", 1));

	query[1].AddToIndex(TSIndexItem("ÏÀÐÈÆ", 1));
	query[1].AddToIndex(TSIndexItem("ÒÅÐÐÎÐÈÑÒÈ×ÅÑÊÈÉ ÀÊÒ", 1));

	TSDocCollection coll;
	data_extractor.GetDocuments(coll, query);*/

	CLogger::Instance()->WriteToLog("End perf check");
    return 0;
}



