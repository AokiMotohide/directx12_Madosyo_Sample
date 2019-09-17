//�R���X�^���g�o�b�t�@�ōs���]��


#include "Application.h"
#include"Dx12Wrapper.h"
#include"PMDRenderer.h"
#include"PMDActor.h"

#ifdef _DEBUG
#include<iostream>
#endif

///@brief �R���\�[����ʂɃt�H�[�}�b�g�t���������\��
///@param format �t�H�[�}�b�g(%d�Ƃ�%f�Ƃ���)
///@param �ϒ�����
///@remarks���̊֐��̓f�o�b�O�p�ł��B�f�o�b�O���ɂ������삵�܂���
void DebugOutputFormatString(const char* format, ...) {
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif
}

//�ʓ|�����Ǐ����Ȃ�������
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_DESTROY) {//�E�B���h�E���j�����ꂽ��Ă΂�܂�
		PostQuitMessage(0);//OS�ɑ΂��āu�������̃A�v���͏I�����v�Ɠ`����
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);//�K��̏������s��
}

//�E�B���h�E�萔
const unsigned int window_width = 1280;
const unsigned int window_height = 720;

void 
Application::CreateGameWindow(HWND &hwnd, WNDCLASSEX &windowClass) {
	HINSTANCE hInst = GetModuleHandle(nullptr);
	//�E�B���h�E�N���X�������o�^
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = (WNDPROC)WindowProcedure;//�R�[���o�b�N�֐��̎w��
	windowClass.lpszClassName = _T("DirectXTest");//�A�v���P�[�V�����N���X��(�K���ł����ł�)
	windowClass.hInstance = GetModuleHandle(0);//�n���h���̎擾
	RegisterClassEx(&windowClass);//�A�v���P�[�V�����N���X(���������̍�邩���낵������OS�ɗ\������)

	RECT wrc = { 0,0, window_width, window_height };//�E�B���h�E�T�C�Y�����߂�
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);//�E�B���h�E�̃T�C�Y�͂�����Ɩʓ|�Ȃ̂Ŋ֐����g���ĕ␳����
	//�E�B���h�E�I�u�W�F�N�g�̐���
	hwnd = CreateWindow(windowClass.lpszClassName,//�N���X���w��
		_T("DX12���t�@�N�^�����O"),//�^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,//�^�C�g���o�[�Ƌ��E��������E�B���h�E�ł�
		CW_USEDEFAULT,//�\��X���W��OS�ɂ��C�����܂�
		CW_USEDEFAULT,//�\��Y���W��OS�ɂ��C�����܂�
		wrc.right - wrc.left,//�E�B���h�E��
		wrc.bottom - wrc.top,//�E�B���h�E��
		nullptr,//�e�E�B���h�E�n���h��
		nullptr,//���j���[�n���h��
		windowClass.hInstance,//�Ăяo���A�v���P�[�V�����n���h��
		nullptr);//�ǉ��p�����[�^

}



void 
Application::Run() {
	ShowWindow(_hwnd, SW_SHOW);//�E�B���h�E�\��
	float angle = 0.0f;
	MSG msg = {};
	unsigned int frame = 0;
	while (true) {
		//_worldMat = XMMatrixRotationY(angle);
		//_mapScene->world = _worldMat;
		//_mapScene->view = _viewMat;
		//_mapScene->proj = _projMat;
		//angle += 0.01f;

		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//�����A�v���P�[�V�������I�����Ď���message��WM_QUIT�ɂȂ�
		if (msg.message == WM_QUIT) {
			break;
		}

		//�S�̂̕`�揀��
		_dx12->BeginDraw();

		//PMD�p�̕`��p�C�v���C���ɍ��킹��
		_dx12->CommandList()->SetPipelineState(_pmdRenderer->GetPipelineState());
		//���[�g�V�O�l�`����PMD�p�ɍ��킹��
		_dx12->CommandList()->SetGraphicsRootSignature(_pmdRenderer->GetRootSignature());

		_dx12->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		
		
		_pmdActor->Draw();





		_dx12->EndDraw();

		//�t���b�v
		_dx12->Swapchain()->Present(1, 0);

	}
}

bool 
Application::Init() {
	auto result = CoInitializeEx(0, COINIT_MULTITHREADED);
	CreateGameWindow(_hwnd, _windowClass);


	//DirectX12���b�p�[������������
	_dx12.reset(new Dx12Wrapper(_hwnd));
	_pmdRenderer.reset(new PMDRenderer(*_dx12));
	_pmdActor.reset(new PMDActor("Model/�����~�N.pmd", *_pmdRenderer));

//#ifdef _DEBUG
//	//�f�o�b�O���C���[���I����
//	EnableDebugLayer();
//#endif
//
//	//DirectX12�֘A������
//	if (FAILED(InitializeDXGIDevice())) {
//		assert(0);
//		return false;
//	}
//	if (FAILED(InitializeCommand())) {
//		assert(0);
//		return false;
//	}
//	if (FAILED(CreateSwapChain(_hwnd, _dxgiFactory))) {
//		assert(0);
//		return false;
//	}
//	if (FAILED(CreateFinalRenderTarget(_rtvHeaps, _backBuffers))) {
//		assert(0);
//		return false;
//	}
//
//	//�e�N�X�`�����[�_�[�֘A������
//	CreateTextureLoaderTable();


	////�[�x�o�b�t�@�쐬
	//if (FAILED(CreateDepthStencilView())) {
	//	assert(0);
	//	return false;
	//}

	////�t�F���X�̍쐬
	//if (FAILED(_dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf())))) {
	//	assert(0);
	//	return false;
	//}


	//_whiteTex = CreateWhiteTexture();
	//_blackTex = CreateBlackTexture();
	//_gradTex = CreateGrayGradationTexture();

	//LoadPMDFile("Model/hibiki/hibiki.pmd");
	//LoadPMDFile("Model/satori/satori.pmd");
	//LoadPMDFile("Model/reimu/reimu.pmd");
	//LoadPMDFile("Model/�������J.pmd");
	//if (FAILED(LoadPMDFile("Model/�����~�N.pmd"))) {
	//	return false;
	//}

	////���[�h�����f�[�^�����ƂɃo�b�t�@�Ƀ}�e���A���f�[�^�����
	//if (FAILED(CreateMaterialData())) {
	//	return false;
	//}

	////�}�e���A���܂��̃r���[�쐬����уe�N�X�`���r���[�쐬
	////�����f�X�N���v�^�q�[�v���ɍ���Ă������߃}�e���A����
	////����������ԂɂȂ��Ă���B
	//CreateMaterialAndTextureView();

	////���[�g�V�O�l�`���쐬
	//if (FAILED(CreateRootSignature())) {
	//	return false;
	//}

	////�p�C�v���C���ݒ�(�V�F�[�_�������Őݒ�)
	//if (FAILED(CreateBasicGraphicsPipeline())) {
	//	return false;
	//}

	//if (FAILED(CreateSceneTransformView())) {
	//	return false;
	//}
	return true;
}

void
Application::Terminate() {
	//�����N���X�g��񂩂�o�^�������Ă�
	UnregisterClass(_windowClass.lpszClassName, _windowClass.hInstance);
}


Application::Application()
{
}


Application::~Application()
{
}

Application& 
Application::Instance() {
	static Application instance;
	return instance;
}

