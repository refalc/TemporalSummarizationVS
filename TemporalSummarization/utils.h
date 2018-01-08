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

// typedefs
using StringHash = std::array<unsigned long long, 3>;

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
	int m_QEDocCount;
	double m_PSoftOr;
	double m_DIMinLinkScore;
	int m_PKeepL;
	int m_PKeepT;
	double m_PLambda;
	int m_PMinSentSize;
	double m_PMinMMR;
	double m_QEMinDocRank;
	double m_PMaxSentSize;
	double m_PLambdaTemp;
	bool m_PTemporalMode;
	bool m_PMultiQuerry;
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
};

class CLogger
{
public:
	~CLogger();
	static CLogger *Instance();

	template<typename T>
	void WriteToLog(const T &str)
	{
		//todo parallelism
		if (!m_pFile.is_open())
			return;

		time_t cur_time;
		auto time_point = std::chrono::system_clock::now();
		cur_time = std::chrono::system_clock::to_time_t(time_point);
		std::array<char, 26> buffer;
		ctime_s(buffer.data(), buffer.size(), &cur_time);

		m_pFile << std::string(buffer.begin(), buffer.begin() + 24) + " " + std::string(str) << std::endl;
	}

private:
	CLogger();

private:
	static std::unique_ptr<CLogger> m_spInstance;
	const std::string m_sLoggerFile = "C:\\!DEV\\C++\\Diplom\\TemporalSummarization\\TemporalSummarization\\tss.log";
	std::fstream m_pFile;
};

class CArgReader
{
public:
	bool ReadArguments(int argc, char **argv);
	bool ReadArguments(int argc, const std::string *argv);
	bool GetAnswerPath(std::string &answer) const;
	bool GetW2VPath(std::string &w2v_path) const;
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
	std::vector<std::string> m_vQueries;
	Params m_Params;
};

class CProfiler
{
public:
	~CProfiler();
	static CProfiler *Instance();
	void AddDuration(const std::string &mark, double duration);
	void DataToLog();
private:
	CProfiler();

private:
	static std::unique_ptr<CProfiler> m_spInstance;
	std::map<std::string, double> m_Data;
};

class CIndex
{
public:
	~CIndex();
	static CIndex *Instance();
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
	static std::unique_ptr<CIndex> m_spInstance;

	std::unordered_map<std::string, int> m_S2IIndex;
	std::unordered_map<int, std::string> m_I2SIndex;
	const std::string m_sFileName = std::string("C:\\!DEV\\C++\\Diplom\\TemporalSummarization\\build-TemporalSummarization-Desktop_Qt_5_5_1_MinGW_32bit-Release\\index.idx");
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

private:
	std::fstream m_pFileIn;
	std::fstream m_pFileOut;
	std::string m_FilesFolder = "";
};