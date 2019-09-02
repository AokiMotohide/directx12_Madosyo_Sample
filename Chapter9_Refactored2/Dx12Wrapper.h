#pragma once
#include<d3d12.h>
#include<map>
#include<d3dcompiler.h>
#include<wrl.h>
#include<string>
class Dx12Wrapper
{
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	std::map<std::string,ComPtr<ID3D12Resource>> _textureTable;
	//�e�N�X�`�����[�_�e�[�u���̍쐬
	void CreateTextureLoaderTable();
	//�e�N�X�`��������e�N�X�`���o�b�t�@�쐬�A���g���R�s�[
	HRESULT CreateTextureFromFile(const char* texpath);
public:
	Dx12Wrapper();
	~Dx12Wrapper();
	///�e�N�X�`���p�X����K�v�ȃe�N�X�`���o�b�t�@�ւ̃|�C���^��Ԃ�
	///@param texpath �e�N�X�`���t�@�C���p�X
	ComPtr<ID3D12Resource> GetTextureByName(const char* texpath);
	
};

