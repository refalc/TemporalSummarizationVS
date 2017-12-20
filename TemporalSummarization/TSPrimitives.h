#pragma once
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <functional>
#include <optional>
#include <variant>
// consts
constexpr int W2VSize = 100;

// enums
enum class SDataType
{
	LEMMA = 0,
	WORD = 1,
	TERMIN = 2,
	SENT = 3,
	FINAL_TYPE
};

enum class ReplyType {
	ERR = 0,
	DOC = 1,
	DOC_LIST = 2,
};

enum class SMetaDataType
{
	DATE = 0,
	INT_DATE = 1,
	SITE = 2,
	TITLE = 3,
};

// predefs
class TSDocument;
class TSIndex;
//

// typedefs
using ReplyDataType = std::pair<ReplyType, std::string>;
using ProcessedDataType = std::variant<TSDocument, std::vector<std::string>>;
using RequestDataType = std::variant<std::string, std::vector<TSIndex>>;
using IndexItemIDType = std::string;
//

class TSIndexItemID
{
public:
	TSIndexItemID() : m_ID("") {}
	TSIndexItemID(std::string &&id) : m_ID(std::move(id)) {}
	TSIndexItemID(const std::string &id) : m_ID(id) {}
	TSIndexItemID(const TSIndexItemID &other) { m_ID = other.m_ID; }
	TSIndexItemID(TSIndexItemID &&other) { m_ID = std::move(other.m_ID); }

	inline const TSIndexItemID &operator=(const TSIndexItemID &other) { m_ID = other.m_ID; return *this; }
	inline const TSIndexItemID &operator=(TSIndexItemID &&other) { m_ID = std::move(other.m_ID); return *this; }

	bool operator==(const TSIndexItemID &other) const;
	bool operator<(const TSIndexItemID &other) const;

private:
	std::string m_ID;
};

class TSIndexItem
{
public:
	TSIndexItem(const IndexItemIDType &id, float w, std::vector<int> &&positions) :
		m_ID(id),
		m_fWeight(w),
		m_Positions(std::move(positions)) {}

	TSIndexItem(const TSIndexItem &other);
	TSIndexItem(TSIndexItem &&other);

	const TSIndexItem &operator=(const TSIndexItem &other);
	const TSIndexItem &operator=(TSIndexItem &&other);

	inline IndexItemIDType &GetID() { return m_ID; }
	inline const IndexItemIDType &GetID() const { return m_ID; }
	inline float &GetWeight() { return m_fWeight; }
	inline float GetWeight() const { return m_fWeight; }
	inline std::vector<int> &GetPositions() { return m_Positions; }
	inline const std::vector<int> &GetPositions() const { return m_Positions; }

	inline bool operator==(const TSIndexItem &rhs) const { return m_ID == rhs.m_ID; }

private:
	IndexItemIDType m_ID;
	float m_fWeight;
	std::vector<int> m_Positions;
};

class TSIndex
{
public:
	TSIndex(SDataType type) : m_eIndexType(type) {}
	TSIndex(const TSIndex &other);
	TSIndex(TSIndex &&other);

	const TSIndex &operator=(const TSIndex &other);
	const TSIndex &operator=(TSIndex &&other);

	template<typename T>
	bool AddToIndex(T &&item) {
		m_Index.emplace_back(std::forward<T>(item));
		return true;
	}

	inline SDataType GetType() const { return m_eIndexType; }

	// index iteration
	inline std::vector<TSIndexItem>::iterator begin() { return m_Index.begin(); }
	inline std::vector<TSIndexItem>::iterator end() { return m_Index.end(); }
	inline std::vector<TSIndexItem>::const_iterator begin() const { return m_Index.begin(); }
	inline std::vector<TSIndexItem>::const_iterator end() const { return m_Index.end(); }


private:
	SDataType m_eIndexType;
	std::vector<TSIndexItem> m_Index;
};

class TSSentence
{
public:
	TSSentence(const TSDocument *doc, int sentence_num, int start_pos, int end_pos) :
		m_pDoc(doc),
		m_iSentenceNum(sentence_num),
		m_iStartPos(start_pos),
		m_iEndPos(end_pos) {}

	TSSentence(const TSSentence &other);
	TSSentence(TSSentence &&other);

	const TSSentence &operator=(const TSSentence &other);
	const TSSentence &operator=(TSSentence &&other);

	inline void AddBoundaries(int start_pos, int end_pos) { m_iStartPos = start_pos; m_iEndPos = end_pos; }
	inline bool IsCorrectBoundaries() const { return m_iStartPos >= 0 && m_iEndPos > m_iStartPos; }
	bool AddIndexItem(const TSIndexItem &index_item, SDataType type);
	

private:
	std::vector<TSIndex> m_Indexies;
	const TSDocument *m_pDoc;
	int m_iSentenceNum;
	int m_iStartPos;
	int m_iEndPos;

};

class TSMetaData
{
public:
	TSMetaData() {}
	TSMetaData(const TSMetaData &other);
	TSMetaData(TSMetaData &&other);

	const TSMetaData &operator=(const TSMetaData &other);
	const TSMetaData &operator=(TSMetaData &&other);


	bool AddData(SMetaDataType type, std::string &&data);

private:
	int ConstructIntDate(const std::string &data);

private:
	std::map<SMetaDataType, std::string> m_Data;
};

class TSDocument
{
public:
	TSDocument() {}
	TSDocument(const std::string &id) : m_DocID(id) {}
	TSDocument(const TSDocument &other);
	TSDocument(TSDocument &&other);

	const TSDocument &operator=(const TSDocument &other);
	const TSDocument &operator=(TSDocument &&other);

	bool InitDocID(const std::string &doc_id);
	bool AddSentence(int sentence_num, int start_pos, int end_pos);
	bool AddIndexItem(TSIndexItem &&index_item, const std::vector<int> &sentences, SDataType type);
	inline bool AddMetaData(SMetaDataType type, std::string &&data) { return m_MetaData.AddData(type, std::move(data)); }

	inline const std::string &GetDocID() const { return m_DocID; };

private:
	std::string m_DocID;
	std::vector<TSIndex> m_Indexies;
	std::vector<TSSentence> m_Sentences;
	TSMetaData m_MetaData;
};

class TSDocCollection
{
public:
	bool AddDocToCollection(TSDocument &&doc);

private:
	std::map<std::string, TSDocument> m_Docs;
};

class IReplyProcessor
{
public:
	virtual ~IReplyProcessor() {}
	virtual bool ProcessReply(const ReplyDataType &reply,  ProcessedDataType &data) const = 0;
};

class ISearchEngine
{
public:
	virtual ~ISearchEngine() {}
	virtual bool InitParameters(const std::initializer_list<float> &params) = 0;
	virtual bool SendRequest(const RequestDataType &request, ReplyDataType &reply) = 0;
	virtual IReplyProcessor *GetReplyProcessor() = 0;
};