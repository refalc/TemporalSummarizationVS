#include "utils.h"
#include <windows.h>
#include <iostream>
#include <locale>
#include "TSPrimitives.h"

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
		if (argc < 4) {
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
			} else if( !cmd.compare("-d") ) {
				if( !m_sDocsSerializationPath.empty() ) {
					CLogger::Instance()->WriteToLog("Incorrect command arguments");
					return false;
				}
				m_sDocsSerializationPath = data;
			} else if( !cmd.compare("-t") ) {
				if( !m_sTopicModelingPath.empty() ) {
					CLogger::Instance()->WriteToLog("Incorrect command arguments");
					return false;
				}
				m_sTopicModelingPath = data;
			} else {
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
	cur_params.m_DIW2VEnable = false;
	cur_params.m_SlvW2VEnable = false;
	cur_params.m_MaxAnswerSize = 15;            //todo task_param
	cur_params.m_PMinSentSize = 4;              //todo hardcode
	cur_params.m_PMinMMR = 0.0001;              //todo hardcode
	cur_params.m_PowerMethodEps = 0.0000001;    //todo hardcode
	cur_params.m_SentSimThreshold = 0.8;

	if (!GetDataFromTag(content, "PDocCount", cur_params.m_PDocCount))
		return false;
	if (!GetDataFromTag(content, "QEQuerrySize", cur_params.m_QEQuerrySize))
		return false;
	if (!GetDataFromTag(content, "QETopLemms", cur_params.m_QETopLemms))
		return false;
	if( !GetDataFromTag(content, "QETopTermins", cur_params.m_QETopTermins) )
		return false;
	if (!GetDataFromTag(content, "QEDocCount", cur_params.m_QEDocCount))
		return false;
	if (!GetDataFromTag(content, "PSoftOr", cur_params.m_PSoftOr))
		return false;
	if( !GetDataFromTag(content, "QESoftOr", cur_params.m_QESoftOr) )
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
	if( !GetDataFromTag(content, "DIW2VEnable", cur_params.m_DIW2VEnable) )
		return false;
	if( !GetDataFromTag(content, "SlvW2VEnable", cur_params.m_SlvW2VEnable) )
		return false;
	if( !GetDataFromTag(content, "DIIsClusterization", cur_params.m_DIIsClusterization) )
		return false;
	if( !GetDataFromTag(content, "DIIsPyramidFeature", cur_params.m_DIIsPyramidFeature) )
		return false;
	if( !GetDataFromTag(content, "DIIsLexRank", cur_params.m_DIIsLexRank) )
		return false;
	if( !GetDataFromTag(content, "DIClusterizationSimThreshold", cur_params.m_DIClusterizationSimThreshold) )
		return false;
	if( !GetDataFromTag(content, "DIMaxHourDiff", cur_params.m_DIMaxHourDiff) )
		return false;
	if( !GetDataFromTag(content, "DITopKValue", cur_params.m_DITopKValue) )
		return false;
	if( !GetDataFromTag(content, "IsTopicModeling", cur_params.m_IsTopicModeling) )
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

bool CArgReader::GetDocsSerializationPath(std::string &docs_serialization_path) const
{
	if( m_sDocsSerializationPath.empty() )
		return false;

	docs_serialization_path = m_sDocsSerializationPath;
	return true;
}

bool CArgReader::GetTopicModelingPath(std::string &topic_modeling_path) const
{
	if( m_sTopicModelingPath.empty() )
		return false;

	topic_modeling_path = m_sTopicModelingPath;
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
CProfiler::~CProfiler()
{
	DataToLog();
}

CProfiler::CProfiler()
{

}

void CProfiler::AddDuration(const std::string &mark, double duration)
{
#pragma omp critical (profiler_add_duration) 
	{
		if( m_Data.find(mark) == m_Data.end() )
			m_Data.emplace(mark, duration);
		else
			m_Data[mark] += duration;
	}
}

void CProfiler::DataToLog()
{
	for( const auto &elem : m_Data ) {
		CLogger::Instance()->WriteToLog(elem.first + " : " + std::to_string(elem.second) + " sec");
	}
}
//--------------------PROFILER--------------------

//--------------------INDEX--------------------
CIndex::CIndex()
{
	m_iLastIndex = -1;

	if( !Load() ) {
		std::cout << "Error to load index from " + m_sFileName << std::endl;
		CLogger::Instance()->WriteToLog("Error to load index from " + m_sFileName);
	}

	/*std::set<std::string> vStopWords;
	vStopWords.insert("�");
	vStopWords.insert("���");
	vStopWords.insert("��");
	vStopWords.insert("��");
	vStopWords.insert("�");
	vStopWords.insert("��");
	vStopWords.insert("��");
	vStopWords.insert("�");
	vStopWords.insert("��");
	vStopWords.insert("�����");
	vStopWords.insert("���");
	vStopWords.insert("�����");
	vStopWords.insert("�");
	vStopWords.insert("�");
	vStopWords.insert("��");
	vStopWords.insert("���");
	vStopWords.insert("��");
	vStopWords.insert("���");
	vStopWords.insert("���");
	vStopWords.insert("���");
	vStopWords.insert("�");
	vStopWords.insert("���");
	vStopWords.insert("�");
	vStopWords.insert("��");
	vStopWords.insert("���");
	vStopWords.insert("MDASH");
	vStopWords.insert("RAQUO");
	vStopWords.insert("LAQUO");
	vStopWords.insert("��");
	vStopWords.insert("�");
	vStopWords.insert("��");
	vStopWords.insert("��");
	vStopWords.insert("QUOT");
	vStopWords.insert("NDASH");
	vStopWords.insert("NBSP");

	for( const auto &elem : vStopWords ) {
		m_StopWords.insert(AddToIndex(elem));
	}*/
}

CIndex::~CIndex()
{
	if( !Save() ) {
		std::cout << "Error save file " + m_sFileName << std::endl;
		CLogger::Instance()->WriteToLog("Error save file " + m_sFileName);
	}
}

bool CIndex::GetStr(int ID, std::string &str) const
{
	str.clear();

	const auto term_iter = m_I2SIndex.find(ID);
	if( term_iter != m_I2SIndex.end() ) {
		str = term_iter->second;
		return true;
	}

	return false;

}
int CIndex::AddToIndex(const std::string &str)
{
	if( str.empty() )
		return -1;

	int index = GetID(str);
	if( index == -1 ) {
#pragma omp critical (index_add_to_index) 
		{
			index = GetID(str);
			if( index == -1) {
				m_iLastIndex++;
				m_S2IIndex[str] = m_iLastIndex;
				m_I2SIndex[m_iLastIndex] = str;
				index = m_iLastIndex;
			}
		}
	}
	return index;
}

int CIndex::GetID(const std::string &str) const
{
	const auto term_iter = m_S2IIndex.find(str);
	if( term_iter != m_S2IIndex.end() )
		return term_iter->second;

	return -1;
}
bool CIndex::IsStopWord(const std::string &term) const
{
	return IsStopWord(GetID(term));
}

bool CIndex::IsStopWord(const int term_id) const
{
	if( m_StopWords.find(term_id) != m_StopWords.end() )
		return true;

	return false;
}

bool CIndex::Load()
{
	std::fstream pFile;
	pFile.open(m_sFileName, std::fstream::in);
	if( !pFile.is_open() )
		return false;

	std::string str;
	if( !std::getline(pFile, str) )
		return false;

	int curr_index = 0;
	while( std::getline(pFile, str) ) {
		//str = utils::converter::Utf8_to_cp1251(str.data());
		m_S2IIndex[str] = curr_index;
		m_I2SIndex[curr_index] = str;

		curr_index++;
	}

	m_iLastIndex = curr_index - 1;

	pFile.close();

	return true;
}

bool CIndex::Save()
{
	std::fstream pFile;
	pFile.open(m_sFileName, std::fstream::out);
	if( !pFile.is_open() )
		return false;

	pFile << m_S2IIndex.size() << std::endl;
	for( int i = 0; i < m_S2IIndex.size(); i++ )
		pFile << m_I2SIndex[i] << std::endl;

	pFile.close();

	return true;
}
//--------------------INDEX--------------------

//--------------------HISTORY--------------------
HistoryController::HistoryController(const std::string &file_folder)
{
	m_FilesFolder = file_folder;/* +doc_id + ".ser";*/
}

HistoryController::~HistoryController()
{
	if( m_pFileIn.is_open() )
		m_pFileIn.close();
	if( m_pFileOut.is_open() )
		m_pFileOut.close();
}

void HistoryController::SaveMode(const std::string &doc_id)
{
	std::string file_name = m_FilesFolder + doc_id + ".ser";
	if( !m_pFileOut.is_open() ) {
		if( m_pFileIn.is_open() )
			m_pFileIn.close();

		m_pFileOut.open(file_name, std::ios::out | std::ios::trunc | std::ios::binary);
		if( !m_pFileOut.is_open() ) {
			std::cout << "Error opening file to save " << file_name << std::endl;
		}
	}
}

void HistoryController::LoadMode(const std::string &doc_id)
{
	std::string file_name = m_FilesFolder + doc_id + ".ser";
	if( !m_pFileIn.is_open() ) {
		if( m_pFileOut.is_open() )
			m_pFileOut.close();

		m_pFileIn.open(file_name, std::ios::in | std::ios::binary);
		if( !m_pFileIn.is_open() ) {
			std::cout << "Error opening file to save " << file_name << std::endl;
			return;
		}

		m_pFileIn.seekg(0, std::ios::end);
		std::streampos	length = m_pFileIn.tellg();
		m_pFileIn.seekg(0, std::ios::beg);

		m_LoadBuffer.resize(length);
		std::fill(m_LoadBuffer.begin(), m_LoadBuffer.end(), 0);

		m_pFileIn.read(&m_LoadBuffer[0], length);
		m_pFileIn.seekg(0, std::ios::beg);

		m_LoadStream.SetBuffer(&m_LoadBuffer[0], length);
	}
}

void HistoryController::CloseFiles()
{
	if( m_pFileIn.is_open() ) {
		m_LoadBuffer.clear();
		m_pFileIn.close();
	}

	if( m_pFileOut.is_open() )
		m_pFileOut.close();
}

void HistoryController::operator<<(const uint32_t &data)
{
	m_pFileOut.write((char *)&data, sizeof(uint32_t));
}

void HistoryController::operator<<(const double &data)
{
	m_pFileOut.write((char *)&data, sizeof(double));
}

void HistoryController::operator<<(const std::string &data)
{
	*this << (uint32_t)data.size();
	m_pFileOut << data;
}

void HistoryController::operator>>(uint32_t &data)
{
	//m_pFileIn.read((char *)&data, sizeof(uint32_t));
	m_LoadStream.sgetn((char *)&data, sizeof(uint32_t));
}

void HistoryController::operator>>(double &data)
{
	//m_pFileIn.read((char *)&data, sizeof(double));
	m_LoadStream.sgetn((char *)&data, sizeof(double));
}

void HistoryController::operator>>(std::string &data)
{
	uint32_t str_size;
	*this >> str_size;
	data.resize(str_size);
	//m_pFileIn.read((char *)data.c_str(), data.size());
	m_LoadStream.sgetn((char *)data.c_str(), data.size());
}

void HistoryController::flush()
{
	m_pFileOut.flush();
}
//--------------------HISTORY--------------------

//--------------------W2V------------------------
bool Word2Vec::Load(const std::string &path)
{
	m_Model.clear();

	std::ifstream pFile;
	pFile.open(path, std::ios_base::binary);
	if( !pFile.is_open() ) {
		CLogger::Instance()->WriteToLog("ERROR : Error while load w2v model");
		return false;
	}

	int words = 0, size = 0;
	pFile >> words;
	pFile >> size;
	if( size != W2V_VECTOR_SIZE ) {
		CLogger::Instance()->WriteToLog("ERROR : Error while load w2v model, incorrect vector size");
		return false;
	}

	std::cout << "vocab = " << words << "\nsize = " << size << std::endl;

	auto string_toupper = [](std::string &s, const std::locale &loc)
	{
		for( std::string::iterator i = s.begin(), j = s.end(); i != j; ++i )
			*i = std::toupper(*i, loc);
	};

	char ch;
	std::string word_str;
	auto loc = std::locale("rus");
	for( int i = 0; i < words; i++ ) {
		pFile >> word_str;
		word_str = utils::converter::Utf8_to_cp1251(word_str.data());

		string_toupper(word_str, loc);

		pFile.read(&ch, 1);

		auto iter = m_Model.emplace(word_str, std::array<float, W2V_VECTOR_SIZE>());

		float temp_val = 0.;
		for( int j = 0; j < size; j++ ) {
			pFile.read((char*)&temp_val, 1 * sizeof(float));
			iter.first->second[j] = temp_val;
		}
	}

	pFile.close();

	return true;
}
//--------------------W2V------------------------

//--------------------TopicModel------------------------
TopicModel::TopicModel(float threshold) :
	m_fLoadThreshold(threshold)
{
}

bool TopicModel::Load(const std::string &path)
{
	std::cout << "Load topic model\n";

	m_Model.clear();
	std::fstream pfile;
	pfile.open(path, std::ios::in | std::ios::binary);

	if( !pfile.is_open() )
		return false;

	int32_t row_size;
	pfile.read(reinterpret_cast<char *>(&row_size), sizeof(row_size));
	int j = 0, last_j = 0;
	auto last_time = std::chrono::high_resolution_clock::now();
	size_t row_lengh = row_size * sizeof(double) + sizeof(int32_t);
	std::vector<char> buffer(row_lengh);
	while( !pfile.eof() ) {
		pfile.read(buffer.data(), row_lengh);
		if( j % 50000 == 0 ) {
			auto curr_time = std::chrono::high_resolution_clock::now();
			double duration = (double)std::chrono::duration_cast<std::chrono::microseconds>(curr_time - last_time).count() / 1e6;
			std::cout << '\r' << j << ' ' << (j - last_j) / duration << "d/s" << '\t';
			last_j = j;
			last_time = curr_time;
		}
		int32_t doc_index;
		memcpy(&doc_index, buffer.data(), sizeof(doc_index));
		m_Model[doc_index] = std::map<int, float>();
		for( int i = 0; i < row_size; i++ ) {
			double d_val;
			memcpy(&d_val, buffer.data() + sizeof(int32_t) + i * sizeof(double), sizeof(d_val));
			if( d_val < m_fLoadThreshold )
				continue;

			m_Model[doc_index].emplace(i, (float)d_val);
		}

		memset(buffer.data(), 0, row_lengh);
		j++;
	}

	return true;
}
bool TopicModel::GetDataForDoc(int index, std::vector<TSIndexItem> &data) const
{
	auto iter = m_Model.find(index);
	if( iter == m_Model.end() )
		return false;

	data.reserve(iter->second.size());
	for( const auto elem : iter->second ) {
		std::string topic_id = "topic_" + std::to_string(elem.first);
		data.emplace_back(topic_id, elem.second);
	}
	return true;
}
//--------------------TopicModel------------------------

CPostPrinter::CPostFile::CPostFile(const std::string &file_name, PostFileType type) :
	m_sFileName(file_name),
	m_eFileType(type),
	m_iCallID(0)
{
	if( m_eFileType != CreateAlways && m_eFileType != CreateAlwaysNameCallID ) {
		if( m_eFileType == GlobalApp )
			m_pFile.open(m_sFileName, std::ios::app);
		else if( m_eFileType == LocalApp )
			m_pFile.open(m_sFileName, std::ios::out);
	}
}

bool CPostPrinter::CPostFile::Write(const std::string &str)
{
	if( m_eFileType == CreateAlways )
		m_pFile.open(m_sFileName, std::ios::out);
	else if( m_eFileType == CreateAlwaysNameCallID )
		m_pFile.open(std::to_string(m_iCallID) + "ID_"+ m_sFileName, std::ios::out);

	if( !m_pFile.is_open() )
		return false;

	m_iCallID++;
	m_pFile << str << std::endl;

	if( m_eFileType == CreateAlways || m_eFileType == CreateAlwaysNameCallID )
		m_pFile.close();

	return true;
}

bool CPostPrinter::WriteToFile(const std::string &file_name, PostFileType file_type, const std::string &str)
{
	auto file_iter = m_Files.find(file_name);
	if( file_iter == m_Files.end() ) {
#pragma omp critical (CPostPrinter_add_file) 
		{
			if( file_iter == m_Files.end() ) {
				file_iter = m_Files.emplace(file_name, CPostFile(file_name, file_type)).first;
				m_FilesMutexes.emplace(file_name, std::make_unique<std::mutex>());
			}
		}
	}
	if( file_iter->second.GetType() != file_type )
		return false;

	std::lock_guard<std::mutex> lock(*m_FilesMutexes[file_name]);
	return file_iter->second.Write(str);
}

CPostPrinter::CPostPrinter()
{
}