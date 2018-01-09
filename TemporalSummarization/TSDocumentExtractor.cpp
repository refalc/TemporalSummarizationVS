#include "TSDocumentExtractor.h"
#include "TSDataExtractor.h"
#include <winsock.h>
#include <iostream>

TSDocumentExtractor::TSDocumentExtractor() :
	m_iDocTailSize(4)
{
}


TSDocumentExtractor::~TSDocumentExtractor()
{
}

bool TSDocumentExtractor::InitParameters(const std::initializer_list<float> &params)
{
	constexpr int parameters_size = 6;
	if( params.size() < parameters_size )
		return false;

	int doc_count = (int)params.begin()[0];
	float soft_or = params.begin()[1],
		  min_doc_rank = params.begin()[2],
		  min_link_score = params.begin()[3],
		  power_d_factor = params.begin()[4],
		  importance_boundary = params.begin()[5];

	if( doc_count < 0 || soft_or < 0.0f || soft_or > 1.f ||
		min_doc_rank < 0.0f || min_doc_rank > 1.0f ||
		min_link_score < 0.0f || min_link_score > 1.0f ||
		power_d_factor < 0.0f || power_d_factor > 1.0f ||
		importance_boundary < 0.0f || importance_boundary > 1.0f)
		return false;

	m_iDocCount = doc_count;
	m_fSoftOr = soft_or;
	m_fMinDocRank = min_doc_rank;
	m_fMinLinkScore = min_link_score;
	m_fPowerDFactor = power_d_factor;
	m_fDocumentImportanceBoundary = importance_boundary;

	return true;
}

bool TSDocumentExtractor::ConstructTimeLineCollections(const TSQuery &query, TSTimeLineCollections &collections)
{
	if( m_pDataExtractor->InitParameters({ (float)m_iDocCount, m_fSoftOr, m_fMinDocRank }) != ReturnCode::TS_NO_ERROR )
		return false;

	TSDocCollection whole_collection;
	if( m_pDataExtractor->GetDocuments(query, whole_collection) != ReturnCode::TS_NO_ERROR )
		return false;

	std::map<std::string, float> doc_to_importance;
	std::vector<std::string> top_docs;
	ComputeDocsImportance(whole_collection, doc_to_importance, top_docs);

	if( !SeparateCollectionByTime(whole_collection, collections) )
		return false;

	collections.InitDocumentsImportanceData(std::move(doc_to_importance), std::move(top_docs));
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

void TSDocumentExtractor::ComputeDocsImportance(const TSDocCollection &whole_collection, std::map<std::string, float> &doc_to_importance, std::vector<std::string> &top_docs) const
{
	std::vector<TSDocumentRepresentation> docs_representations;
	ConstructDocumentsRepresentations(whole_collection, docs_representations);

	std::vector<std::vector<float>> similarity_matrix;
	ConstructSimilarityMatrix(docs_representations, similarity_matrix);

	std::vector<float> importances;
	PowerMethod(docs_representations, similarity_matrix, importances);

	for( int i = 0; i < importances.size(); i++ )
		docs_representations[i].SetImportance(importances[i]);

	std::sort(docs_representations.begin(), docs_representations.end(), [](const TSDocumentRepresentation &lhs, const TSDocumentRepresentation &rhs) {
		return lhs.GetImportance() > rhs.GetImportance();
	});

	for( const auto doc_repr : docs_representations ) {
		doc_to_importance.insert(doc_repr.GetPair());
		if( doc_repr.GetImportance() > m_fDocumentImportanceBoundary )
			top_docs.push_back(doc_repr.GetDocPtr()->GetDocID());
	}
}

void TSDocumentExtractor::ConstructSimilarityMatrix(const std::vector<TSDocumentRepresentation> &docs_representations, std::vector<std::vector<float>> &similarity_matrix) const
{
	int size = docs_representations.size();
	similarity_matrix.resize(size);
	for( auto &row : similarity_matrix ) {
		row.resize(size);
		std::fill(row.begin(), row.end(), 0.f);
	}

	for( int i = 0; i < size; i++ ) {
		for( int j = 0; j < size; j++ ) {
			if( j == i )
				continue;
			// if reference from j to i
			if( docs_representations[j].GetDate() > docs_representations[i].GetDate() ) {
				float sim = docs_representations[j] * docs_representations[i];
				if( sim > m_fMinLinkScore )
					similarity_matrix[i][j] = sim;
			}
		}
	}
}

void TSDocumentExtractor::PowerMethod(const std::vector<TSDocumentRepresentation> &docs_representations, const std::vector<std::vector<float>> &similarity_matrix, std::vector<float> &importances) const
{
	int size = docs_representations.size();
	importances.resize(size);

	std::vector<float> importance_next(size, m_fPowerDFactor / size), importance_last(size, m_fPowerDFactor / size);

	std::vector<int> degrees(size, 0);
	for( int i = 0; i < size; i++ )
		degrees[i] = std::count_if(similarity_matrix[i].begin(), similarity_matrix[i].end(), [](const float &elem) { return elem > 0.f; });

	float max_dif = size, epsilon = 0.001;

	while( max_dif > epsilon ) {
		max_dif = 0;
		// calculate importance_next
		for( int i = 0; i < size; i++ ) {
			importance_next[i] = m_fPowerDFactor / size;
			for( auto j = 0; j < size; j++ ) 
				importance_next[i] += (1.f - m_fPowerDFactor) * degrees[j] * importance_last[j];

			float dif = std::fabs(importance_next[i] - importance_last[i]);
			max_dif = max(max_dif, dif);
		}
		importance_last.swap(importance_next);
	}

	importances.swap(importance_last);
}

void TSDocumentExtractor::ConstructDocumentsRepresentations(const TSDocCollection &whole_collection, std::vector<TSDocumentRepresentation> &docs_representations) const
{
	docs_representations.resize(whole_collection.size());
	int i = 0;
	for( const auto &doc_pair : whole_collection ) {
		ConstructDocRepresentation(doc_pair.second, docs_representations[i]);
		i++;
	}
}

void TSDocumentExtractor::ConstructDocRepresentation(const TSDocument &document, TSDocumentRepresentation &doc_representation) const
{
	doc_representation.InitDocPtr(&document);

	int i = 0;
	for( auto sentences_iter = document.sentences_begin(); sentences_iter != document.sentences_end() && i < m_iDocHeadSize; sentences_iter++, i++ ) {
		TSSentenceConstPtr sentence_ptr = &(*sentences_iter);
		doc_representation.AddHead(sentence_ptr);
	}

	i = 0;
	for( auto sentences_iter = document.sentences_rbegin(); sentences_iter != document.sentences_rend() && i < m_iDocTailSize; sentences_iter++, i++ ) {
		TSSentenceConstPtr sentence_ptr = &(*sentences_iter);
		if( IsSentenceHasReferenceToThePast(sentence_ptr) )
			doc_representation.AddTail(sentence_ptr);
	}
}

bool TSDocumentExtractor::IsSentenceHasReferenceToThePast(TSSentenceConstPtr sentence) const
{
	return sentence->GetDocPtr()->sentences_size() - sentence->GetSentenseNumber() < m_iDocTailSize;
}

void TSDocumentExtractor::TSDocumentRepresentation::InitDocPtr(TSDocumentConstPtr doc_ptr)
{
	m_pDoc = doc_ptr;
	std::string date;
	m_pDoc->GetMetaData(SMetaDataType::INT_DATE, date);
	m_iDate = std::stoi(date);
}

void TSDocumentExtractor::TSDocumentRepresentation::AddHead(TSSentenceConstPtr sentence_ptr)
{
	m_HeadSentences.push_back(sentence_ptr);
}

void TSDocumentExtractor::TSDocumentRepresentation::AddTail(TSSentenceConstPtr sentence_ptr)
{
	m_TailSentences.push_back(sentence_ptr);
}

float TSDocumentExtractor::TSDocumentRepresentation::operator*(const TSDocumentRepresentation &other) const
{
	float weight = 0.f;
	for( auto tail_sentence : m_TailSentences ) {
		for( auto head_sentence : other.m_HeadSentences ) {
			weight = max(weight, *tail_sentence * *head_sentence);
		}
	}
	return weight;
}