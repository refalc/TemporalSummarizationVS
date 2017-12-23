#include "TSQueryConstructor.h"
#include "TSDataExtractor.h"
#include <algorithm>

TSQueryConstructor::TSQueryConstructor()
{
}

TSQueryConstructor::~TSQueryConstructor()
{
}

bool TSQueryConstructor::InitParameters(const std::initializer_list<float> &params)
{
	constexpr int parameters_size = 8;
	if( params.size() < parameters_size )
		return false;

	int lemms_size = (int)params.begin()[0],
		terms_size = (int)params.begin()[1],
		doc_count = (int)params.begin()[2];
	
	float soft_or = params.begin()[3],
		  min_doc_rank = params.begin()[4];

	int top_lemm_for_qe = params.begin()[5],
		top_termin_for_qe = params.begin()[6],
		result_lemma_size = params.begin()[7],
		result_termins_size = params.begin()[8];

	if( lemms_size < 1 || terms_size < 0 || doc_count <= 0 ||
		soft_or >= 1 || soft_or < 0 ||
		min_doc_rank > 1 || min_doc_rank  < 0 ||
		top_lemm_for_qe < 1 || top_termin_for_qe < 0 ||
		result_lemma_size < 1 || result_termins_size < 0 )
		return false;


	m_iLemmsFor1L = lemms_size;
	m_iTermsFor1L = terms_size;
	m_iDocCountForQEProcess = doc_count;
	m_fSoftOrForQEProcess = soft_or;
	m_fMinDocRankForQEProcess = min_doc_rank;
	m_iTopLemmsForQEProcess = top_lemm_for_qe;
	m_iTopTerminsForQEProcess = top_termin_for_qe;
	m_iResultLemmaSize = result_lemma_size;
	m_iResultTerminsSize = result_termins_size;

	return true;
}

bool TSQueryConstructor::ConstructQueryFromDoc(const TSDocument &doc, TSQuery &query) const
{
	query.clear();
	query.InitIndex(SDataType::LEMMA);
	query.InitIndex(SDataType::TERMIN);

	TSIndexConstPtr doc_index_ptr;
	TSIndexPtr query_index_ptr;
	if( !doc.GetIndex(SDataType::LEMMA, doc_index_ptr) || !query.GetIndex(SDataType::LEMMA, query_index_ptr) )
		return false;

	CopyFromIndexFirstNItems(*doc_index_ptr, m_iLemmsFor1L, *query_index_ptr);

	if( !doc.GetIndex(SDataType::TERMIN, doc_index_ptr) || !query.GetIndex(SDataType::TERMIN, query_index_ptr) )
		return false;

	CopyFromIndexFirstNItems(*doc_index_ptr, m_iTermsFor1L, *query_index_ptr);

	return true;
}

bool TSQueryConstructor::QueryConstructionProcess(const std::string &doc_id, TSQuery &query) const
{
	if( !m_pDataExtractor )
		return false;

	TSDocument doc;
	if( m_pDataExtractor->GetDocument(doc, doc_id) != ReturnCode::TS_NO_ERROR )
		return false;

	if( !ConstructQueryFromDoc(doc, query) )
		return false;

	return true;
}

bool TSQueryConstructor::QueryExtensionProcess(const TSQuery &query, TSQuery &extended_query) const
{
	auto init_param = std::initializer_list<float>{ (float)m_iDocCountForQEProcess, m_fSoftOrForQEProcess, m_fMinDocRankForQEProcess };
	if( m_pDataExtractor->InitParameters(init_param) != ReturnCode::TS_NO_ERROR )
		return false;

	TSDocCollection collection;
	if( m_pDataExtractor->GetDocuments(collection, query) != ReturnCode::TS_NO_ERROR )
		return false;
	
	extended_query.clear();
	extended_query.InitIndex(SDataType::LEMMA);
	extended_query.InitIndex(SDataType::TERMIN);

	TSIndexPtr query_lemma_index_ptr, query_termins_index_ptr;
	if( !extended_query.GetIndex(SDataType::LEMMA, query_lemma_index_ptr) || !extended_query.GetIndex(SDataType::TERMIN, query_termins_index_ptr) )
		return false;

	query_lemma_index_ptr->reserve(collection.size() * m_iTopLemmsForQEProcess);
	query_termins_index_ptr->reserve(collection.size() * m_iTopTerminsForQEProcess);

	for( auto iter = collection.begin(); iter != collection.end(); iter++ ) {
		TSIndexConstPtr p_index;
		if( !iter->second.GetIndex(SDataType::LEMMA, p_index) )
			return false;

		FillCollectionStatistic(p_index, m_iTopLemmsForQEProcess, *query_lemma_index_ptr);

		if( !iter->second.GetIndex(SDataType::TERMIN, p_index) )
			return false;

		FillCollectionStatistic(p_index, m_iTopTerminsForQEProcess, *query_termins_index_ptr);
	}

	CombineStatistics(*query_lemma_index_ptr, m_iResultLemmaSize);
	CombineStatistics(*query_termins_index_ptr, m_iResultTerminsSize);

	return true;
}

void TSQueryConstructor::CopyFromIndexFirstNItems(const TSIndex &from_index, int n, TSIndex &to_index) const
{
	auto index_iter = from_index.begin();
	for( int i = 0; i < std::min(n, (int)from_index.size()); i++, index_iter++ )
		to_index.AddToIndex(*index_iter);
}

void TSQueryConstructor::FillCollectionStatistic(TSIndexConstPtr p_index, int num_of_element, TSIndex &statistic_index) const
{
	for( int i = 0; i < std::min(num_of_element, (int)p_index->size()); i++ )
		statistic_index.AddToIndex(*(p_index->begin() + i));
}

void TSQueryConstructor::CombineStatistics(TSIndex &statistic_index, int query_size) const
{
	std::sort(statistic_index.begin(), statistic_index.end());

	auto iter_placer = statistic_index.begin(),
		 iter_runner = statistic_index.begin();

	int cur_item_count = 0, all_item_count = (int)statistic_index.size();
	for( ; iter_runner != statistic_index.end(); iter_runner++ ) {
		while( iter_runner != statistic_index.end() && *iter_runner == *iter_placer ) {
			cur_item_count++;
			iter_runner++;
		}

		if( iter_runner == statistic_index.end() )
			break;

		iter_placer->GetWeight() = (float)cur_item_count / all_item_count;
		iter_placer++;
		*iter_placer = *iter_runner;
		cur_item_count = 1;
	}
	iter_placer->GetWeight() = (float)cur_item_count / all_item_count;
	iter_placer++;

	int uniq_query_size = (int)std::distance(statistic_index.begin(), iter_placer);
	std::sort(statistic_index.begin(), iter_placer, [] (const TSIndexItem &lhs, const TSIndexItem &rhs) {
		return lhs.GetWeight() > rhs.GetWeight();
	});

	statistic_index.erase(statistic_index.begin() + std::min(query_size, uniq_query_size), statistic_index.end());
}