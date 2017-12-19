#include "TSPrimitives.h"
#include <algorithm>
#include "utils.h"

bool TSSentence::AddIndexItem(const TSIndexItem &index_item, SDataType type)
{
	TSIndexItem curr_index_item = index_item;
	// todo correct tf-idf

	// correct positions
	curr_index_item.GetPositions().erase(std::remove_if(curr_index_item.GetPositions().begin(), curr_index_item.GetPositions().end(), [this](const int &elem) {
		return elem > m_iEndPos || elem < m_iStartPos; 
	}), curr_index_item.GetPositions().end());

	if( curr_index_item.GetPositions().empty() )
		return false;

	auto index_iter = std::find_if(m_Indexies.begin(), m_Indexies.end(), [type](const TSIndex &index) { return index.GetType() == type; });
	if( index_iter == m_Indexies.end() ) {
		m_Indexies.emplace_back(TSIndex(type));
		index_iter = m_Indexies.begin() + m_Indexies.size() - 1;
	}
	
	return index_iter->AddToIndex(std::move(curr_index_item));
}

bool TSDocument::InitDocID(const std::string &doc_id)
{
	if( !m_DocID.empty() )
		return false;

	m_DocID = doc_id;
	return true;
}

bool TSDocument::AddSentence(int sentence_num, int start_pos, int end_pos)
{
	if( sentence_num < 0 || start_pos < 0 || end_pos <= start_pos )
		return false;

	if( sentence_num >= m_Sentences.size() ) {
		for( int i = (int)m_Sentences.size(); i < sentence_num; i++ )
			m_Sentences.emplace_back(TSSentence(this, i, -1, -1));
		m_Sentences.emplace_back(TSSentence(this, sentence_num, start_pos, end_pos));
	} else {
		m_Sentences[sentence_num].AddBoundaries(start_pos, end_pos);
	}

	return true;
}
bool TSDocument::AddIndexItem(TSIndexItem &&index_item, const std::vector<int> &sentences, SDataType type)
{
	// sentences must be sorted
	if( sentences.empty() || sentences.back() >= m_Sentences.size() )
		return false;

	// fill sentences
	for( const auto &sent_id : sentences ) {
		if( !m_Sentences[sent_id].AddIndexItem(index_item, type) )
			return false;
	}

	// fill doc index
	auto index_iter = std::find_if(m_Indexies.begin(), m_Indexies.end(), [type](const TSIndex &index) { return index.GetType() == type; });
	if (index_iter == m_Indexies.end()) {
		m_Indexies.emplace_back(TSIndex(type));
		index_iter = m_Indexies.begin() + m_Indexies.size() - 1;
	}

	return index_iter->AddToIndex(index_item);
}