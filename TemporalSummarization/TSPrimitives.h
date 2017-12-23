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
class TSIndexiesHolder;

//

// typedefs
using TSQuery = TSIndexiesHolder;
using ReplyDataType = std::pair<ReplyType, std::string>;
using ProcessedDataType = std::variant<TSDocument, std::vector<std::string>>;
using RequestDataType = std::variant<std::string, TSQuery>;
using IndexItemIDType = std::string;
using TSIndexPtr = TSIndex * ;
using TSIndexConstPtr = const TSIndex * ;
//

class TSIndexItemID
{
public:
	TSIndexItemID() : m_ID("") {}
	TSIndexItemID(std::string &&id) : m_ID(std::move(id)) {}
	TSIndexItemID(const std::string &id) noexcept : m_ID(id) {}
	TSIndexItemID(const TSIndexItemID &other) noexcept { m_ID = other.m_ID; }
	TSIndexItemID(TSIndexItemID &&other) noexcept { m_ID = std::move(other.m_ID); }

	inline const TSIndexItemID &operator=(const TSIndexItemID &other) noexcept { m_ID = other.m_ID; return *this; }
	inline const TSIndexItemID &operator=(TSIndexItemID &&other) noexcept { m_ID = std::move(other.m_ID); return *this; }
	inline bool operator==(const TSIndexItemID &other) const noexcept {
		return m_ID.size() == other.m_ID.size() && memcmp(m_ID.data(), other.m_ID.data(), m_ID.size()) == 0;
	}
	inline bool operator<(const TSIndexItemID &other) const noexcept { return m_ID.compare(other.m_ID) < 0; }

private:
	std::string m_ID;
};

class TSIndexItem
{
public:
	TSIndexItem(const IndexItemIDType &id, float w) noexcept :
		m_ID(id),
		m_fWeight(w) {}

	TSIndexItem(const TSIndexItem &other) noexcept;
	TSIndexItem(TSIndexItem &&other) noexcept;

	const TSIndexItem &operator=(const TSIndexItem &other) noexcept;
	const TSIndexItem &operator=(TSIndexItem &&other) noexcept;

	inline IndexItemIDType &GetID() { return m_ID; }
	inline const IndexItemIDType &GetID() const { return m_ID; }
	inline float &GetWeight() { return m_fWeight; }
	inline float GetWeight() const { return m_fWeight; }
	inline bool operator==(const TSIndexItem &rhs) const noexcept { return m_ID == rhs.m_ID; }
	inline bool operator<(const TSIndexItem &rhs) const noexcept { return m_ID < rhs.m_ID; }

private:
	IndexItemIDType m_ID;
	float m_fWeight;
};

class TSIndex
{
public:
	TSIndex(SDataType type) noexcept : m_eIndexType(type) {}
	TSIndex(const TSIndex &other);
	TSIndex(TSIndex &&other) noexcept;

	const TSIndex &operator=(const TSIndex &other);
	const TSIndex &operator=(TSIndex &&other) noexcept;

	template<typename T>
	bool AddToIndex(T &&item) {
		m_Index.push_back(std::forward<T>(item));
		return true;
	}

	inline SDataType GetType() const { return m_eIndexType; }
	std::string GetString() const;
	// index iteration 
	// std notation
	inline std::vector<TSIndexItem>::iterator begin() { return m_Index.begin(); }
	inline std::vector<TSIndexItem>::iterator end() { return m_Index.end(); }
	inline std::vector<TSIndexItem>::const_iterator begin() const { return m_Index.begin(); }
	inline std::vector<TSIndexItem>::const_iterator end() const { return m_Index.end(); }
	inline size_t size() const { return m_Index.size(); }
	void reserve(size_t n) { m_Index.reserve(n); }

	std::vector<TSIndexItem>::iterator erase(std::vector<TSIndexItem>::const_iterator first, std::vector<TSIndexItem>::const_iterator second) {
		return m_Index.erase(first, second);
	}
	std::vector<TSIndexItem>::iterator erase(std::vector<TSIndexItem>::const_iterator pos) {
		return m_Index.erase(pos);
	}
	

private:
	SDataType m_eIndexType;
	std::vector<TSIndexItem> m_Index;
};

class TSIndexiesHolder
{
public:
	bool InitIndex(SDataType type);
	bool GetIndex(SDataType type, TSIndex &index) const;
	bool GetIndex(SDataType type, TSIndexConstPtr &p_index) const;
	bool GetIndex(SDataType type, TSIndexPtr &p_index);

	template<typename T>
	bool AddIndexItemToIndex(SDataType type, T &&item)
	{
		auto index_iter = std::find_if(m_Indexies.begin(), m_Indexies.end(), [type] (const TSIndex &index) {
			return index.GetType() == type; 
		});

		if( index_iter == m_Indexies.end() ) {
			m_Indexies.emplace_back(type);
			index_iter = m_Indexies.begin() + m_Indexies.size() - 1;
		}

		return index_iter->AddToIndex(std::forward<T>(item));
	}

	void clear() { m_Indexies.clear(); }
	inline std::vector<TSIndex>::iterator begin() { return m_Indexies.begin(); }
	inline std::vector<TSIndex>::iterator end() { return m_Indexies.end(); }
	inline std::vector<TSIndex>::const_iterator begin() const { return m_Indexies.begin(); }
	inline std::vector<TSIndex>::const_iterator end() const { return m_Indexies.end(); }

protected:
	std::vector<TSIndex> m_Indexies;
};

class TSSentence : public TSIndexiesHolder
{
public:
	TSSentence(const TSDocument *doc, int sentence_num, int start_pos, int end_pos) noexcept :
		m_pDoc(doc),
		m_iSentenceNum(sentence_num),
		m_iStartPos(start_pos),
		m_iEndPos(end_pos) {}

	TSSentence(const TSSentence &other);
	TSSentence(TSSentence &&other) noexcept;

	const TSSentence &operator=(const TSSentence &other);
	const TSSentence &operator=(TSSentence &&other) noexcept;

	inline void AddBoundaries(int start_pos, int end_pos) { m_iStartPos = start_pos; m_iEndPos = end_pos; }
	inline bool IsCorrectBoundaries() const { return m_iStartPos >= 0 && m_iEndPos > m_iStartPos; }	

private:
	const TSDocument *m_pDoc;
	int m_iSentenceNum;
	int m_iStartPos;
	int m_iEndPos;

};

class TSMetaData
{
public:
	TSMetaData() noexcept {}
	TSMetaData(const TSMetaData &other);
	TSMetaData(TSMetaData &&other) noexcept;

	const TSMetaData &operator=(const TSMetaData &other);
	const TSMetaData &operator=(TSMetaData &&other) noexcept;


	bool AddData(SMetaDataType type, std::string &&data);

private:
	int ConstructIntDate(const std::string &data);

private:
	std::map<SMetaDataType, std::string> m_Data;
};

class TSDocument : public TSIndexiesHolder
{
public:
	TSDocument() noexcept {}
	TSDocument(const std::string &id) noexcept : m_DocID(id) {}
	TSDocument(const TSDocument &other);
	TSDocument(TSDocument &&other) noexcept;

	const TSDocument &operator=(const TSDocument &other);
	const TSDocument &operator=(TSDocument &&other) noexcept;

	bool InitDocID(const std::string &doc_id);
	bool AddSentence(int sentence_num, int start_pos, int end_pos);
	bool AddIndexItem(TSIndexItem &&index_item, const std::vector<int> &sentences, SDataType type);
	inline bool AddMetaData(SMetaDataType type, std::string &&data) { return m_MetaData.AddData(type, std::move(data)); }

	inline const std::string &GetDocID() const { return m_DocID; };
	inline bool IsEmpty() const { return m_Indexies.empty(); };

private:
	std::string m_DocID;
	std::vector<TSSentence> m_Sentences;
	TSMetaData m_MetaData;
};

class TSDocCollection
{
public:
	bool AddDocToCollection(TSDocument &&doc);

	inline size_t size() const { return m_Docs.size(); }
	// iterate docs
	inline std::map<std::string, TSDocument>::iterator begin() { return m_Docs.begin(); }
	inline std::map<std::string, TSDocument>::iterator end() { return m_Docs.end(); }
	inline std::map<std::string, TSDocument>::const_iterator begin() const { return m_Docs.begin(); }
	inline std::map<std::string, TSDocument>::const_iterator end() const { return m_Docs.end(); }

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