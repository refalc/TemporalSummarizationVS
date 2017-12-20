#include "TSPrimitives.h"
#include <algorithm>
#include "utils.h"

bool TSIndexItemID::operator==(const TSIndexItemID &other) const
{
	if( m_ID.size() != other.m_ID.size() )
		return false;

	return memcmp(m_ID.data(), other.m_ID.data(), m_ID.size()) == 0;
}

bool TSIndexItemID::operator<(const TSIndexItemID &other) const
{
	return m_ID.compare(other.m_ID) < 0;
}

TSIndexItem::TSIndexItem(const TSIndexItem &other)
{
	*this = other;
}

TSIndexItem::TSIndexItem(TSIndexItem &&other)
{
	*this = std::move(other);
}

const TSIndexItem &TSIndexItem::operator=(const TSIndexItem &other)
{
	m_fWeight = other.m_fWeight;
	m_ID = other.m_ID;
	m_Positions = other.m_Positions;

	return *this;
}

const TSIndexItem &TSIndexItem::operator=(TSIndexItem &&other)
{
	m_fWeight = std::move(other.m_fWeight);
	m_ID = std::move(other.m_ID);
	m_Positions = std::move(other.m_Positions);

	return *this;
}

TSIndex::TSIndex(const TSIndex &other)
{
	*this = other;
}

TSIndex::TSIndex(TSIndex &&other)
{
	*this = std::move(other);
}

const TSIndex &TSIndex::operator=(const TSIndex &other)
{
	m_eIndexType = other.m_eIndexType;
	m_Index = other.m_Index;

	return *this;
}

const TSIndex &TSIndex::operator=(TSIndex &&other)
{
	m_eIndexType = std::move(other.m_eIndexType);
	m_Index = std::move(other.m_Index);

	return *this;
}

TSSentence::TSSentence(const TSSentence &other)
{
	*this = other;
}

TSSentence::TSSentence(TSSentence &&other)
{
	*this = std::move(other);
}

const TSSentence &TSSentence::operator=(const TSSentence &other)
{
	m_pDoc = other.m_pDoc;
	m_iSentenceNum = other.m_iSentenceNum;
	m_iStartPos = other.m_iStartPos;
	m_iEndPos = other.m_iEndPos;
	m_Indexies = m_Indexies;

	return *this;
}

const TSSentence &TSSentence::operator=(TSSentence &&other)
{
	m_pDoc = std::move(other.m_pDoc);
	m_iSentenceNum = std::move(other.m_iSentenceNum);
	m_iStartPos = std::move(other.m_iStartPos);
	m_iEndPos = std::move(other.m_iEndPos);
	m_Indexies = std::move(m_Indexies);

	return *this;
}

bool TSSentence::AddIndexItem(const TSIndexItem &index_item, SDataType type)
{
	TSIndexItem curr_index_item = index_item;

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

TSMetaData::TSMetaData(const TSMetaData &other)
{
	*this = other;
}

TSMetaData::TSMetaData(TSMetaData &&other)
{
	*this = std::move(other);
}

const TSMetaData &TSMetaData::operator=(const TSMetaData &other)
{
	m_Data = other.m_Data;

	return *this;
}

const TSMetaData &TSMetaData::operator=(TSMetaData &&other)
{
	m_Data = std::move(other.m_Data);

	return *this;
}


TSDocument::TSDocument(const TSDocument &other)
{
	*this = other;
}

TSDocument::TSDocument(TSDocument &&other)
{
	*this = std::move(other);
}

const TSDocument &TSDocument::operator=(const TSDocument &other)
{
	m_DocID = other.m_DocID;
	m_Indexies = other.m_Indexies;
	m_Sentences = other.m_Sentences;
	m_MetaData = other.m_MetaData;

	return *this;
}

const TSDocument &TSDocument::operator=(TSDocument &&other)
{
	m_DocID = std::move(other.m_DocID);
	m_Indexies = std::move(other.m_Indexies);
	m_Sentences = std::move(other.m_Sentences);
	m_MetaData = std::move(other.m_MetaData);

	return *this;
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

	return index_iter->AddToIndex(std::move(index_item));
}

bool TSMetaData::AddData(SMetaDataType type, std::string &&data)
{
	if( type == SMetaDataType::DATE ) {
		int idate = ConstructIntDate(data);
		if( idate >= 0 )
			m_Data[SMetaDataType::INT_DATE] = std::to_string(idate);
		else
			CLogger::Instance()->WriteToLog("ERROR: Incorrect date format : " + data);
	}

	m_Data[type] = std::move(data);

	return true;
}

int TSMetaData::ConstructIntDate(const std::string &data)
{
	std::string day = data.substr(0, 2);
	std::string month = data.substr(3, 2);
	std::string year = data.substr(6, 4);
	if( !utils::IsStringIntNumber(day) || !utils::IsStringIntNumber(month) || !utils::IsStringIntNumber(year) )
		return -1;

	return std::stoi(day) + std::stoi(month) * 31 + std::stoi(year) * 365;
}

bool TSDocCollection::AddDocToCollection(TSDocument &&doc)
{
	const std::string &doc_id = doc.GetDocID();
	auto doc_iter = m_Docs.lower_bound(doc_id);
	if( doc_iter == m_Docs.end() || m_Docs.key_comp()(doc_id, doc_iter->first) ) {
		m_Docs.emplace_hint(doc_iter, std::make_pair(doc_id, std::move(doc)));
		return true;
	} 

	// doc with this doc_id already exist
	return false;
}