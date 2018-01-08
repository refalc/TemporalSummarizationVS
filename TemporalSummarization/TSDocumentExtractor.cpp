#include "TSDocumentExtractor.h"
#include "TSDataExtractor.h"
#include <winsock.h>
#include <iostream>

TSDocumentExtractor::TSDocumentExtractor()
{
}


TSDocumentExtractor::~TSDocumentExtractor()
{
}

bool TSDocumentExtractor::InitParameters(const std::initializer_list<float> &params)
{
	constexpr int parameters_size = 3;
	if( params.size() < parameters_size )
		return false;

	int doc_count = (int)params.begin()[0];
	float soft_or = params.begin()[1],
		  min_doc_rank = params.begin()[2];

	if( doc_count < 0 || soft_or < 0.0f || soft_or > 1.f ||
		min_doc_rank < 0.0f || min_doc_rank  > 1.0f )
		return false;

	m_iDocCount = doc_count;
	m_fSoftOr = soft_or;
	m_fMinDocRank = min_doc_rank;

	return true;
}

bool TSDocumentExtractor::ConstructTimeLineCollections(const TSQuery &query, TSTimeLineCollections &collections)
{
	if( m_pDataExtractor->InitParameters({ (float)m_iDocCount, m_fSoftOr, m_fMinDocRank }) != ReturnCode::TS_NO_ERROR )
		return false;

	TSDocCollection whole_collection;
	if( m_pDataExtractor->GetDocuments(query, whole_collection) != ReturnCode::TS_NO_ERROR )
		return false;

	if( !SeparateCollectionByTime(whole_collection, collections) )
		return false;

	return true;
}

bool TSDocumentExtractor::SeparateCollectionByTime(TSDocCollection &whole_collection, TSTimeLineCollections &collections) const
{
	while( whole_collection.size() > 0 ) {
		auto doc_iter = whole_collection.begin();
		std::string s_int_date;
		if( !doc_iter->second.GetMetaData(SMetaDataType::INT_DATE, s_int_date) )
			return false;

		collections.AddDocNode(whole_collection.ExtractNode(doc_iter->first), std::stoi(s_int_date));
	}
	return true;
}

void TSDocumentExtractor::ComputeDocsImportance(TSTimeLineCollections &collections) const
{

}

void TSDocumentExtractor::ConstructSimilarityMatrix() const
{

}
void TSDocumentExtractor::PowerMethod() const
{

}