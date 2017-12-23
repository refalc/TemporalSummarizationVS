#include "utils.h"
#include <windows.h>

// !!copy from outside
// todo rewrite
namespace utils {
namespace converter {
std::string cp1251_to_utf8(const char *str)
{
	std::string res;
	int result_u, result_c;

	result_u = MultiByteToWideChar(1251, 0, str, -1, 0, 0);

	if (!result_u)
		return 0;

	wchar_t *ures = new wchar_t[result_u];

	if (!MultiByteToWideChar(1251, 0, str, -1, ures, result_u)) {
		delete[] ures;
		return 0;
	}


	result_c = WideCharToMultiByte(CP_UTF8, 0, ures, -1, 0, 0, 0, 0);

	if (!result_c) {
		delete[] ures;
		return 0;
	}

	char *cres = new char[result_c];

	if (!WideCharToMultiByte(CP_UTF8, 0, ures, -1, cres, result_c, 0, 0)) {
		delete[] cres;
		return 0;
	}
	delete[] ures;
	res.append(cres);
	delete[] cres;
	return res;
}

std::string Utf8_to_cp1251(const char *str)
{
	std::string res;
	int result_u, result_c;

	result_u = MultiByteToWideChar(CP_UTF8, 0, str, -1, 0, 0);

	if (!result_u)
		return 0;

	wchar_t *ures = new wchar_t[result_u];

	if (!MultiByteToWideChar(CP_UTF8, 0, str, -1, ures, result_u)) {
		delete[] ures;
		return 0;
	}


	result_c = WideCharToMultiByte(1251, 0, ures, -1, 0, 0, 0, 0);

	if (!result_c) {
		delete[] ures;
		return 0;
	}

	char *cres = new char[result_c];

	if (!WideCharToMultiByte(1251, 0, ures, -1, cres, result_c, 0, 0)) {
		delete[] cres;
		return 0;
	}
	delete[] ures;
	res.append(cres);
	delete[] cres;
	return res;
}
}

namespace hash {
char symb2num(char symb)
{
	if( symb < 0 && symb > -65 )
		return symb + 64;
	if( symb > 47 && symb < 58 )
		return symb + 16;
	if( symb > 64 && symb < 91 )
		return symb + 9;
	if( symb > 96 && symb < 123 )
		return symb + 3;

	return -1;
}

char num2sym(char num)
{
	if (num >= 0 && num < 64)
		return num - 64;
	if (num > 63 && num < 74)
		return num - 16;
	if (num > 73 && num < 100)
		return num - 9;
	if (num > 99 && num < 126)
		return num - 3;

	return 0;
}

bool string2hash(const std::string &str, StringHash &hash)
{
	if (str.size() > 24)
		return false;

	constexpr unsigned long long base = 128;
	unsigned long long curr_multiplier = 1, curr_value = 0;
	int hash_position = 0;
	StringHash temp_hash;

	for (int i = 0; i < str.size(); i++) {
		if (i % 8 == 0 && i != 0) {
			temp_hash[hash_position] = curr_value;
			hash_position++;
			curr_value = 0;
			curr_multiplier = 1;
		}

		char symb_hash = symb2num(str[i]) + 1;
		if (symb_hash == -1)
			return false;

		curr_value += symb_hash * curr_multiplier;
		curr_multiplier *= base;
	}

	temp_hash[hash_position] = curr_value;
	for (int i = hash_position + 1; i < temp_hash.size(); i++)
		temp_hash[i] = 0;

	hash = temp_hash;

	return true;
}

bool hash2string(const StringHash &hash, std::string &str)
{
	constexpr unsigned long long base = 128;
	std::string temp_str;

	for( int i = 0; i < hash.size(); i++ ) {
		unsigned long long curr_value = hash[i];
		while( curr_value ) {
			temp_str += num2sym(curr_value % base - 1);
			curr_value /= base;
		}
	}

	str = temp_str;

	return true;
}

}

bool IsStringFloatNumber(const std::string &str)
{
	bool dot_avialable = true;
	for( int i = 0; i < str.size(); i++ ) {
		if( str[i] <= 0 )
			return false;
		if( !isdigit(str[i]) ) {
			if( str[i] == '.' && dot_avialable && i > 0 && i < str.size() - 1 ) {
				dot_avialable = false;
				continue;
			}
			return false;
		}
	}

	return true;
}

bool IsStringIntNumber(const std::string &str) 
{
	bool dot_avialable = true;
	for( int i = 0; i < str.size(); i++ )
		if( str[i] <= 0  || !isdigit(str[i]) )
			return false;

	return true;
}

}
//

//--------------------ARG READER--------------------
bool CArgReader::ReadArguments(int argc, const std::string *argv)
{
	CLogger::Instance()->WriteToLog("Reading args...");
	try {
		int last_arg_pos = 1;
		if (argc < 3) {
			CLogger::Instance()->WriteToLog("Incorrect command arguments");
			return false;
		}

		int queries_size = std::stoi(argv[last_arg_pos]);
		last_arg_pos += 1;

		if (argc < last_arg_pos + queries_size) {
			CLogger::Instance()->WriteToLog("Incorrect command arguments");
			return false;
		}

		for (int i = 0; i < queries_size; i++)
			m_vQueries.emplace_back(argv[i + last_arg_pos]);

		last_arg_pos += queries_size;
		int rest_params = argc - last_arg_pos;
		if (rest_params % 2) {
			CLogger::Instance()->WriteToLog("Incorrect command arguments");
			return false;
		}

		for (int i = 0; i < rest_params / 2; i++) {
			std::string cmd = argv[last_arg_pos + i * 2],
				data = argv[last_arg_pos + i * 2 + 1];

			if (cmd.empty() || data.empty()) {
				CLogger::Instance()->WriteToLog("Incorrect command arguments");
				return false;
			}

			if (!cmd.compare("-a")) {
				if (!m_sAnswerPath.empty()) {
					CLogger::Instance()->WriteToLog("Incorrect command arguments");
					return false;
				}
				m_sAnswerPath = data;
			}
			else if (!cmd.compare("-e")) {
				if (!m_sW2VPath.empty()) {
					CLogger::Instance()->WriteToLog("Incorrect command arguments");
					return false;
				}
				m_sW2VPath = data;
				//all_w2v.Load(m_sW2VPath);
			}
			else if (!cmd.compare("-c")) {
				if (!m_sConfigPath.empty()) {
					CLogger::Instance()->WriteToLog("Incorrect command arguments");
					return false;
				}
				m_sConfigPath = data;
				if (!ReadConfig(m_sConfigPath)) {
					CLogger::Instance()->WriteToLog("Incorrect config");
					return false;
				}
			}
			else {
				CLogger::Instance()->WriteToLog("Incorrect command arguments");
				return false;
			}
		}

	}
	catch (...) {
		CLogger::Instance()->WriteToLog("Exeption in ReadArguments");
		return false;
	}

	return true;
}

bool CArgReader::ReadArguments(int argc, char **argv)
{
	std::vector<std::string> converted_argv(argc);
	for (int i = 0; i < argc; i++)
		converted_argv[i] = argv[i];

	return ReadArguments((int)converted_argv.size(), converted_argv.data());
}

bool CArgReader::GetStrDataFromTag(const std::string &content, const std::string &tag, std::string &data) const
{
	std::string open_tag = "<" + tag + ">", close_tag = "</" + tag + ">";
	int beg_tag_pos = (int)content.find(open_tag), end_tag_pos = (int)content.find(close_tag);
	if (beg_tag_pos == std::string::npos || end_tag_pos == std::string::npos)
		return false;

	int data_begin_pos = beg_tag_pos + (int)open_tag.size(),
		data_len = end_tag_pos - data_begin_pos;

	data = content.substr(data_begin_pos, data_len);
	CLogger::Instance()->WriteToLog("Param " + tag + " = " + data);

	return true;
}

bool CArgReader::GetDataFromTag(const std::string &content, const std::string &tag, double &data) const
{
	std::string str_data;
	if (!GetStrDataFromTag(content, tag, str_data))
		return false;

	try {
		data = std::stod(str_data);
	}
	catch (...) {
		CLogger::Instance()->WriteToLog("Espected double in config with tag = " + tag);
		return false;
	}

	return true;
}

bool CArgReader::GetDataFromTag(const std::string &content, const std::string &tag, int &data) const
{
	std::string str_data;
	if (!GetStrDataFromTag(content, tag, str_data))
		return false;

	try {
		data = std::stoi(str_data);
	}
	catch (...) {
		CLogger::Instance()->WriteToLog("Espected int in config with tag = " + tag);
		return false;
	}

	return true;
}
bool CArgReader::GetDataFromTag(const std::string &content, const std::string &tag, bool &data) const
{
	std::string str_data;
	if (!GetStrDataFromTag(content, tag, str_data))
		return false;
	if (!str_data.compare("false")) {
		data = false;
	}
	else if (!str_data.compare("true")) {
		data = true;
	}
	else {
		CLogger::Instance()->WriteToLog("Espected false or true in config with tag = " + tag);
		return false;
	}

	return true;
}

bool CArgReader::ReadConfig(std::string path_to_config)
{
	std::fstream pConfigFile;
	pConfigFile.open(path_to_config);
	if (!pConfigFile.is_open())
		return false;

	std::string content;
	content.assign(std::istreambuf_iterator<char>(pConfigFile), std::istreambuf_iterator<char>());

	Params cur_params;
	// perma const params
	cur_params.m_MaxAnswerSize = 15;            //todo task_param
	cur_params.m_PMinSentSize = 4;              //todo hardcode
	cur_params.m_PMinMMR = 0.0001;              //todo hardcode
	cur_params.m_PMaxSentSize = 50;             //todo hardcode
	cur_params.m_PLambdaTemp = 0;               //todo del
	cur_params.m_PMultiQuerry = false;          //todo del
	cur_params.m_PowerMethodEps = 0.0000001;    //todo hardcode
	cur_params.m_SentSimThreshold = 0.8;

	if (!GetDataFromTag(content, "PDocCount", cur_params.m_PDocCount))
		return false;
	if (!GetDataFromTag(content, "QEQuerrySize", cur_params.m_QEQuerrySize))
		return false;
	if (!GetDataFromTag(content, "QETopLemms", cur_params.m_QETopLemms))
		return false;
	if (!GetDataFromTag(content, "QEDocCount", cur_params.m_QEDocCount))
		return false;
	if (!GetDataFromTag(content, "PSoftOr", cur_params.m_PSoftOr))
		return false;
	if (!GetDataFromTag(content, "DIMinLinkScore", cur_params.m_DIMinLinkScore))
		return false;
	if (!GetDataFromTag(content, "PKeepL", cur_params.m_PKeepL))
		return false;
	if (!GetDataFromTag(content, "PKeepT", cur_params.m_PKeepT))
		return false;
	if (!GetDataFromTag(content, "PLambda", cur_params.m_PLambda))
		return false;
	if (!GetDataFromTag(content, "QEMinDocRank", cur_params.m_QEMinDocRank))
		return false;
	if (!GetDataFromTag(content, "PTemporalMode", cur_params.m_PTemporalMode))
		return false;
	if (!GetDataFromTag(content, "DocImportance", cur_params.m_DocImportance))
		return false;
	if (!GetDataFromTag(content, "DIAlpha", cur_params.m_DIAlpha))
		return false;
	if (!GetDataFromTag(content, "DIPowerMethodDFactor", cur_params.m_DIPowerMethodDFactor))
		return false;
	if (!GetDataFromTag(content, "TempMaxDailyAnswerSize", cur_params.m_TempMaxDailyAnswerSize))
		return false;
	if (!GetDataFromTag(content, "DIDocBoundary", cur_params.m_DIDocBoundary))
		return false;
	if (!GetDataFromTag(content, "PQuerryEx", cur_params.m_PQuerryEx))
		return false;
	if (!GetDataFromTag(content, "QEDEInitQuerrySize", cur_params.m_QEDEInitQuerrySize))
		return false;
	if (!GetDataFromTag(content, "QEDoubleExtension", cur_params.m_QEDoubleExtension))
		return false;

	m_Params = cur_params;

	return true;
}

bool CArgReader::GetAnswerPath(std::string &answer) const
{
	if (m_sAnswerPath.empty())
		return false;

	answer = m_sAnswerPath;
	return true;
}

bool CArgReader::GetW2VPath(std::string &w2v_path) const
{
	if (m_sW2VPath.empty())
		return false;

	w2v_path = m_sW2VPath;
	return true;
}

bool CArgReader::GetQueries(std::vector<std::string> &queries) const
{
	if (m_vQueries.empty())
		return false;

	queries = m_vQueries;
	return true;
}

bool CArgReader::GetParams(Params &params) const
{
	params = m_Params;
	return true;
}

//--------------------ARG READER--------------------
//--------------------LOGGER--------------------

std::unique_ptr<CLogger> CLogger::m_spInstance = nullptr;
CLogger *CLogger::Instance()
{
	if (!m_spInstance) {
		// mutex for parallelism
		// lock
		if (!m_spInstance) {
			m_spInstance.reset(new CLogger());
		}
	}

	return m_spInstance.get();
}

CLogger::CLogger()
{
	m_pFile.open(m_sLoggerFile, std::fstream::app);
}
CLogger::~CLogger()
{
	if (m_pFile.is_open())
		m_pFile.close();
}
//--------------------LOGGER--------------------

//--------------------PROFILER--------------------
std::unique_ptr<CProfiler> CProfiler::m_spInstance = nullptr;
CProfiler::~CProfiler()
{
}

CProfiler::CProfiler()
{

}

CProfiler *CProfiler::Instance()
{
	// todo parallelism
	if( !m_spInstance ) {
		m_spInstance.reset(new CProfiler());
	}

	return m_spInstance.get();
}

void CProfiler::AddDuration(const std::string &mark, double duration)
{
	if( m_Data.find(mark) == m_Data.end() )
		m_Data.emplace(mark, duration);
	else
		m_Data[mark] += duration;
}

void CProfiler::DataToLog()
{
	for( const auto &elem : m_Data ) {
		CLogger::Instance()->WriteToLog(elem.first + " : " + std::to_string(elem.second) + " sec");
	}
}
//--------------------PROFILER--------------------