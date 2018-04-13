#pragma once
#include <string>
#include <fstream>
#include <memory>
#include <vector>
#include <chrono>
#include <array>
#include <map>
#include <unordered_map>
#include <set>
#include <chrono>
#include <omp.h>  
#include <queue>
#include <mutex>
#include <sstream>

// consts
constexpr int W2V_VECTOR_SIZE = 100;

// typedefs
using StringHash = std::array<unsigned long long, 3>;
using W2VModelType = std::unordered_map<std::string, std::array<float, W2V_VECTOR_SIZE>>;

// functions
namespace utils {
	namespace converter {
		std::string cp1251_to_utf8(const char *str);
		std::string Utf8_to_cp1251(const char *str);
	}

	namespace hash {
		char symb2num(char symb);
		char num2sym(char num);

		bool string2hash(const std::string &str, StringHash &hash);
		bool hash2string(const StringHash &hash, std::string &str);
	}
	bool IsStringFloatNumber(const std::string &str);
	bool IsStringIntNumber(const std::string &str);
}


struct Params
{
	int m_MaxAnswerSize;
	int m_PDocCount;
	int m_QEQuerrySize;
	int m_QETopLemms;
	int m_QETopTermins;
	int m_QEDocCount;
	double m_QESoftOr;
	double m_PSoftOr;
	double m_DIMinLinkScore;
	int m_PKeepL;
	int m_PKeepT;
	double m_PLambda;
	int m_PMinSentSize;
	double m_PMinMMR;
	double m_QEMinDocRank;
	bool m_PTemporalMode;
	bool m_DocImportance;
	bool m_PQuerryEx;
	double m_DIAlpha;
	double m_PowerMethodEps;
	double m_DIPowerMethodDFactor;
	int m_TempMaxDailyAnswerSize;
	double m_DIDocBoundary;
	int m_QEDEInitQuerrySize;
	bool m_QEDoubleExtension;
	double m_SentSimThreshold;
	bool m_DIW2VEnable;
	bool m_SlvW2VEnable;
	bool m_DIIsClusterization;
	bool m_DIIsPyramidFeature;
	bool m_DIIsLexRank;
	double m_DIClusterizationSimThreshold;
	double m_DIMaxHourDiff;
	int m_DITopKValue;
};

#define CRIT_SECTION_NAME(type) type##_instance
#define DECLARE_FRIEND_FOR_SINGETON(type) friend class SingletonClass<type>;

template<class T>
class SingletonClass
{
public:
	static T* Instance() {
		if( !m_spInstance ) {
#pragma omp critical (CRIT_SECTION_NAME(T)) 
			{
				if( !m_spInstance ) {
					m_spInstance.reset(new T());
				}
			}
		}

		return m_spInstance.get();
	}
private:
	static std::unique_ptr<T> m_spInstance;
};

template<class T>
std::unique_ptr<T> SingletonClass<T>::m_spInstance = nullptr;

class CLogger : public SingletonClass<CLogger>
{
public:
	DECLARE_FRIEND_FOR_SINGETON(CLogger)
	~CLogger();

	template<typename T>
	void WriteToLog(const T &str)
	{
		if( !m_pFile.is_open() )
			return;
#pragma omp critical (logger_write)  
		{
			time_t cur_time;
			auto time_point = std::chrono::system_clock::now();
			cur_time = std::chrono::system_clock::to_time_t(time_point);
			std::array<char, 26> buffer;
			ctime_s(buffer.data(), buffer.size(), &cur_time);

			m_pFile << std::string(buffer.begin(), buffer.begin() + 24) + " | thread " + std::to_string(omp_get_thread_num()) + " | " + std::string(str) << std::endl;
		}
	}

private:
	CLogger();

private:
	const std::string m_sLoggerFile = "tss.log";
	std::fstream m_pFile;
};

class CArgReader
{
public:
	bool ReadArguments(int argc, char **argv);
	bool ReadArguments(int argc, const std::string *argv);
	bool GetAnswerPath(std::string &answer) const;
	bool GetW2VPath(std::string &w2v_path) const;
	bool GetDocsSerializationPath(std::string &docs_serialization_path) const;
	bool GetQueries(std::vector<std::string> &queries) const;
	bool GetParams(Params &params) const;

private:
	bool ReadConfig(std::string path_to_config);
	bool GetStrDataFromTag(const std::string &content, const std::string &tag, std::string &data) const;
	bool GetDataFromTag(const std::string &content, const std::string &tag, double &data) const;
	bool GetDataFromTag(const std::string &content, const std::string &tag, int &data) const;
	bool GetDataFromTag(const std::string &content, const std::string &tag, bool &data) const;

private:
	std::string m_sAnswerPath;
	std::string m_sW2VPath;
	std::string m_sConfigPath;
	std::string m_sDocsSerializationPath;
	std::vector<std::string> m_vQueries;
	Params m_Params;
};

class CProfiler : public SingletonClass<CProfiler>
{
public:
	DECLARE_FRIEND_FOR_SINGETON(CProfiler)
	class CProfilerProbe
	{
	public:
		CProfilerProbe(const std::string &mark) : m_Mark(mark), m_CreationTime(std::chrono::high_resolution_clock::now()) {}
		~CProfilerProbe() 
		{
			double duration = (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_CreationTime).count() / 1e6;
			CProfiler::Instance()->AddDuration(m_Mark, duration);
		}

	private:
		std::string m_Mark;
		std::chrono::time_point<std::chrono::steady_clock> m_CreationTime;
	};

	~CProfiler();
	void AddDuration(const std::string &mark, double duration);
	void DataToLog();
private:
	CProfiler();

private:
	std::map<std::string, double> m_Data;
};

class CIndex : public SingletonClass<CIndex>
{
public:
	DECLARE_FRIEND_FOR_SINGETON(CIndex)
	~CIndex();
	bool GetStr(int ID, std::string &str) const;
	int AddToIndex(const std::string &str);
	int GetID(const std::string &str) const;
	bool IsStopWord(const std::string &term) const;
	bool IsStopWord(const int term_id) const;

private:
	CIndex();
	bool Load();
	bool Save();

private:
	std::unordered_map<std::string, int> m_S2IIndex;
	std::unordered_map<int, std::string> m_I2SIndex;
	const std::string m_sFileName = std::string("index.idx");
	std::set<int> m_StopWords;
	int m_iLastIndex;
};

class HistoryController
{
public:
	HistoryController(const std::string &file_folder);
	~HistoryController();

	void SaveMode(const std::string &doc_id);
	void LoadMode(const std::string &doc_id);
	void CloseFiles();
	void flush();

	void operator<<(const uint32_t &data);
	void operator<<(const double &data);
	void operator<<(const std::string &data);

	void operator>>(uint32_t &data);
	void operator>>(double &data);
	void operator>>(std::string &data);

	class hcstringbuf : public std::basic_streambuf<char, std::char_traits<char> >
	{
	public:
		hcstringbuf &SetBuffer(char_type* buffer, std::streamsize bufferLength) {
			setp(buffer, buffer + bufferLength);
			setg(buffer, buffer, buffer + bufferLength);
			return *this;
		}
	};

private:
	std::fstream m_pFileIn;
	std::fstream m_pFileOut;
	std::string m_FilesFolder = "";
	std::vector<char> m_LoadBuffer;
	hcstringbuf m_LoadStream;
};

class Word2Vec
{
public:
	bool Load(const std::string &path);
	inline const W2VModelType& GetModel() const { return m_Model; }
private:
	W2VModelType m_Model;
};

// for access to container in priority_queue
template <class T, class S, class C>
auto GetPrivateContainer(const std::priority_queue<T, S, C>& q) {
	struct HackedQueue : private std::priority_queue<T, S, C> {
		static const S& GetContainer(const std::priority_queue<T, S, C>& q) {
			return q.*&HackedQueue::c;
		}
	};
	return HackedQueue::GetContainer(q);
}

class CPostPrinter : public SingletonClass<CPostPrinter>
{
public:
	DECLARE_FRIEND_FOR_SINGETON(CPostPrinter)
	enum PostFileType {
		GlobalApp = 0,
		LocalApp = 1,
		CreateAlways = 2,
		CreateAlwaysNameCallID = 3
	};

	class CPostFile
	{
	public:
		CPostFile(const std::string &file_name, PostFileType type);
		inline PostFileType GetType() const { return m_eFileType; }
		bool Write(const std::string &str);

	private:
		std::fstream m_pFile;
		std::string m_sFileName;
		PostFileType m_eFileType;
		int m_iCallID;
	};

	bool WriteToFile(const std::string &file_name, PostFileType file_type, const std::string &str);

private:
	CPostPrinter();

private:
	std::map<std::string, CPostFile> m_Files;
	std::map<std::string, std::unique_ptr<std::mutex>> m_FilesMutexes;
};