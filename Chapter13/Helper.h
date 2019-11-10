#pragma once

#include<d3d12.h>

class Helper
{
public:
	Helper();
	~Helper();
};

///���U���g���`�F�b�N���A�_����������false��Ԃ�
///@param result DX�֐�����̖߂�l
///@param errBlob �G���[������Ȃ�G���[���o��
///@remarks �f�o�b�O���ɂ�errBlob���f�o�b�O�o�͂��s��
///���̂܂܃N���b�V������
extern bool CheckResult(HRESULT &result, ID3DBlob* errBlob=nullptr);

///�A���C�����g���l��Ԃ�
///@param size �A���C�����g�Ώۂ̃T�C�Y
///@param alignment �A���C�����g�T�C�Y
///@retval �A���C�����g����Ă��܂����T�C�Y
extern unsigned int AligmentedValue(unsigned int size, unsigned int alignment = 16);


//�P�o�C�gstring�����C�h����wstring�ɕϊ�����
std::wstring WStringFromString(const std::string& str);

///�g���q��Ԃ�
///@param path ���̃p�X������
///@return �g���q������
std::wstring GetExtension(const std::wstring& path);