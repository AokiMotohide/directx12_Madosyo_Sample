#pragma once

#include<d3d12.h>
#include<vector>
#include<wrl.h>

class PMDActor
{
private:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	ComPtr< ID3D12DescriptorHeap> _materialHeap = nullptr;//�}�e���A���q�[�v(5�Ԃ�)
public:
	PMDActor(const char* filepath);
	~PMDActor();
	PMDActor* Clone();
	void Update();
	void Draw();
};

