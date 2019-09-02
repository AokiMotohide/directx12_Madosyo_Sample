#pragma once

#include<d3d12.h>
#include<DirectXMath.h>
#include<vector>
#include<wrl.h>

class PMDActor
{
private:

	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	//�V�F�[�_���ɓ�������}�e���A���f�[�^
	struct MaterialForHlsl {
		DirectX::XMFLOAT3 diffuse; //�f�B�t���[�Y�F
		float alpha; // �f�B�t���[�Y��
		DirectX::XMFLOAT3 specular; //�X�y�L�����F
		float specularity;//�X�y�L�����̋���(��Z�l)
		DirectX::XMFLOAT3 ambient; //�A���r�G���g�F
	};
	//����ȊO�̃}�e���A���f�[�^
	struct AdditionalMaterial {
		std::string texPath;//�e�N�X�`���t�@�C���p�X
		int toonIdx; //�g�D�[���ԍ�
		bool edgeFlg;//�}�e���A�����̗֊s���t���O
	};
	//�܂Ƃ߂�����
	struct Material {
		unsigned int indicesNum;//�C���f�b�N�X��
		MaterialForHlsl material;
		AdditionalMaterial additional;
	};


	std::vector<Material> _materials;
	std::vector<ComPtr<ID3D12Resource>> _textureResources;
	std::vector<ComPtr<ID3D12Resource>> _sphResources;
	std::vector<ComPtr<ID3D12Resource>> _spaResources;
	std::vector<ComPtr<ID3D12Resource>> _toonResources;
	
	ComPtr<ID3D12Resource> _vb=nullptr;
	ComPtr<ID3D12Resource> _ib=nullptr;
	D3D12_VERTEX_BUFFER_VIEW _vbv = {};
	D3D12_INDEX_BUFFER_VIEW _ibv = {};
	ComPtr< ID3D12DescriptorHeap> _materialHeap = nullptr;//�}�e���A���q�[�v(5�Ԃ�)



	//�}�e���A�����e�N�X�`���̃r���[���쐬
	void CreateMaterialAndTextureView();
	//���W�ϊ��p�r���[�̐���
	HRESULT CreateTransformView();

	bool LoadPMD(const char* filepath);

public:
	PMDActor(const char* filepath);
	~PMDActor();
	PMDActor* Clone();
	void Update();
	void Draw();
};

