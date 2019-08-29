#pragma once

#include<d3d12.h>
#include<vector>
#include<wrl.h>
#include<memory>

class PMDActor;
class PMDRenderer
{
	
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	ComPtr< ID3D12PipelineState> _pipeline = nullptr;//PMD�p�p�C�v���C��
	ComPtr< ID3D12RootSignature> _rootSignature = nullptr;//PMD�p���[�g�V�O�l�`��
public:
	PMDRenderer();
	~PMDRenderer();
	void Update();
	void Draw();
};

