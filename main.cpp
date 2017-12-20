#include <iostream>
#include "TemporalSummarization\TSDataExtractor.h"
#include <fstream>
#include "TemporalSummarization\utils.h"

int main(int argc, char *argv[])
{
	//std::locale::global(std::locale("Russian"));
	CLogger::Instance()->WriteToLog("Start perf check");
	TSDataExtractor data_extractor;
	if( !data_extractor.InitParameters({ 500, 0.3f, 0.1f }) )
		return 0;
	/*for( int i = 0; i < 500; i++ ) {
		TSDocument doc;
		data_extractor.GetDocument(doc, "11633162");
	}*/
	std::vector<TSIndex> query;
	query.push_back(TSIndex(SDataType::LEMMA));
	query.push_back(TSIndex(SDataType::TERMIN));

	query[0].AddToIndex(TSIndexItem("ïàðèæ", 1, {}));
	query[0].AddToIndex(TSIndexItem("òåðàêò", 1, {}));
	query[0].AddToIndex(TSIndexItem("êàðèêàòóðà", 1, {}));

	query[1].AddToIndex(TSIndexItem("ÏÀÐÈÆ", 1, {}));
	query[1].AddToIndex(TSIndexItem("ÒÅÐÐÎÐÈÑÒÈ×ÅÑÊÈÉ ÀÊÒ", 1, {}));

	TSDocCollection coll;
	data_extractor.GetDocuments(coll, query);

	CLogger::Instance()->WriteToLog("End perf check");
    return 0;
}



