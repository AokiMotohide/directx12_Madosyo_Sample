#include<string>
#include<assert.h>
#include "Helper.h"


using namespace std;

Helper::Helper()
{
}


Helper::~Helper()
{
}

//�P�o�C�gstring�����C�h����wstring�ɕϊ�����
wstring WStringFromString(const std::string& str) {
	wstring wstr;
	auto wcharNum = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), nullptr, 0);
	wstr.resize(wcharNum);
	wcharNum = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(),
		&wstr[0], wstr.size());
	return wstr;
}

///�g���q��Ԃ�
///@param path ���̃p�X������
///@return �g���q������
wstring GetExtension(const wstring& path) {
	int index = path.find_last_of(L'.');
	return path.substr(index + 1, path.length() - index);
}

bool CheckResult(HRESULT &result, ID3DBlob * errBlob)
{
	if (FAILED(result)) {
#ifdef _DEBUG
		if (errBlob!=nullptr) {
			std::string outmsg;
			outmsg.resize(errBlob->GetBufferSize());
			std::copy_n(static_cast<char*>(errBlob->GetBufferPointer()),
				errBlob->GetBufferSize(),
				outmsg.begin());
			OutputDebugString(outmsg.c_str());//�o�̓E�B���h�E�ɏo�͂��Ă�
		}
		assert(SUCCEEDED(result));
#endif
		return false;
	}
	else {
		return true;
	}
}

unsigned int
AligmentedValue(unsigned int size, unsigned int alignment) {
	return (size + alignment - (size%alignment));
}