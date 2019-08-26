#pragma once
///�V���O���g���N���X
class Application
{
private:
	//�����ɕK�v�ȕϐ�(�o�b�t�@��q�[�v�Ȃ�)������

	//���V���O���g���̂��߂ɃR���X�g���N�^��private��
	//����ɃR�s�[�Ƒ�����֎~��
	Application();
	Application(const Application&) = delete;
	void operator=(const Application&) = delete;
public:
	///Application�̃V���O���g���C���X�^���X�𓾂�
	static Application& Instance();

	///������
	void Init();

	///���[�v�N��
	void Run();

	///�㏈��
	void Terminate();

	~Application();
};

