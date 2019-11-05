#include "PMDActor.h"
#include"PMDRenderer.h"
#include"Dx12Wrapper.h"
#include<d3dx12.h>
#include<sstream>
#include<array>
using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

#pragma comment(lib,"winmm.lib")

namespace {
	///�e�N�X�`���̃p�X���Z�p���[�^�����ŕ�������
	///@param path �Ώۂ̃p�X������
	///@param splitter ��؂蕶��
	///@return �����O��̕�����y�A
	pair<string, string>
		SplitFileName(const std::string& path, const char splitter = '*') {
		int idx = path.find(splitter);
		pair<string, string> ret;
		ret.first = path.substr(0, idx);
		ret.second = path.substr(idx + 1, path.length() - idx - 1);
		return ret;
	}
	///�t�@�C��������g���q���擾����
	///@param path �Ώۂ̃p�X������
	///@return �g���q
	string
		GetExtension(const std::string& path) {
		int idx = path.rfind('.');
		return path.substr(idx + 1, path.length() - idx - 1);
	}
	///���f���̃p�X�ƃe�N�X�`���̃p�X���獇���p�X�𓾂�
	///@param modelPath �A�v���P�[�V�������猩��pmd���f���̃p�X
	///@param texPath PMD���f�����猩���e�N�X�`���̃p�X
	///@return �A�v���P�[�V�������猩���e�N�X�`���̃p�X
	std::string GetTexturePathFromModelAndTexPath(const std::string& modelPath, const char* texPath) {
		//�t�@�C���̃t�H���_��؂��\��/�̓��ނ��g�p�����\��������
		//�Ƃ�����������\��/�𓾂���΂����̂ŁA�o����rfind���Ƃ��r����
		//int�^�ɑ�����Ă���̂͌�����Ȃ������ꍇ��rfind��epos(-1��0xffffffff)��Ԃ�����
		int pathIndex1 = modelPath.rfind('/');
		int pathIndex2 = modelPath.rfind('\\');
		auto pathIndex = max(pathIndex1, pathIndex2);
		auto folderPath = modelPath.substr(0, pathIndex + 1);
		return folderPath + texPath;
	}


	///Z�������̕������������s���Ԃ��֐�
	///@param lookat ���������������x�N�g��
	///@param up ��x�N�g��
	///@param right �E�x�N�g��
	XMMATRIX LookAtMatrix(const XMVECTOR& lookat, XMFLOAT3& up, XMFLOAT3& right) {
		//��������������(z��)
		XMVECTOR vz = lookat;

		//(�������������������������Ƃ���)����y���x�N�g��
		XMVECTOR vy = XMVector3Normalize(XMLoadFloat3(&up));

		//(�������������������������Ƃ���)y��
		//XMVECTOR vx = XMVector3Normalize(XMVector3Cross(vz, vx));
		XMVECTOR vx = XMVector3Normalize(XMVector3Cross(vy, vz));
		vy = XMVector3Normalize(XMVector3Cross(vz, vx));

		///LookAt��up�����������������Ă���right��ō�蒼��
		if (abs(XMVector3Dot(vy, vz).m128_f32[0]) == 1.0f) {
			//����X�������`
			vx = XMVector3Normalize(XMLoadFloat3(&right));
			//�������������������������Ƃ���Y�����v�Z
			vy = XMVector3Normalize(XMVector3Cross(vz, vx));
			//�^��X�����v�Z
			vx = XMVector3Normalize(XMVector3Cross(vy, vz));
		}
		XMMATRIX ret = XMMatrixIdentity();
		ret.r[0] = vx;
		ret.r[1] = vy;
		ret.r[2] = vz;
		return ret;
	}

	///����̃x�N�g�������̕����Ɍ����邽�߂̍s���Ԃ�
	///@param origin ����̃x�N�g��
	///@param lookat ��������������
	///@param up ��x�N�g��
	///@param right �E�x�N�g��
	///@retval ����̃x�N�g�������̕����Ɍ����邽�߂̍s��
	XMMATRIX LookAtMatrix(const XMVECTOR& origin, const XMVECTOR& lookat, XMFLOAT3& up, XMFLOAT3& right) {
		return XMMatrixTranspose(LookAtMatrix(origin, up, right))*
									LookAtMatrix(lookat, up, right);
	}
	//�{�[�����
	enum class BoneType {
		Rotation,//��]
		RotAndMove,//��]���ړ�
		IK,//IK
		Undefined,//����`
		IKChild,//IK�e���{�[��
		RotationChild,//��]�e���{�[��
		IKDestination,//IK�ڑ���
		Invisible//�����Ȃ��{�[��
	};

}

void
PMDActor::LookAt(float x, float y, float z) {
	_localMat = LookAtMatrix(XMLoadFloat3(&XMFLOAT3(x, y, z)), XMFLOAT3(0, 1, 0), XMFLOAT3(1, 0, 0));
}


void
PMDActor::SolveLookAt(const PMDIK& ik) {
	//���̊֐��ɗ������_�Ńm�[�h�͂ЂƂ����Ȃ��A�`�F�[���ɓ����Ă���m�[�h�ԍ���
	//IK�̃��[�g�m�[�h�̂��̂Ȃ̂ŁA���̃��[�g�m�[�h����^�[�Q�b�g�Ɍ������x�N�g�����l����΂悢
	auto rootNode=_boneNodeAddressArray[ik.nodeIdxes[0]];
	auto targetNode = _boneNodeAddressArray[ik.boneIdx];

	auto opos1 = XMLoadFloat3(&rootNode->startPos);
	auto tpos1 = XMLoadFloat3(&targetNode->startPos);

	auto opos2 = XMVector3TransformCoord( opos1,_boneMatrices[ik.nodeIdxes[0]]);
	auto tpos2 = XMVector3TransformCoord( tpos1, _boneMatrices[ik.boneIdx]);


	auto originVec = XMVectorSubtract(tpos1,opos1);
	auto targetVec = XMVectorSubtract(tpos2,opos2);

	originVec = XMVector3Normalize(originVec);
	targetVec = XMVector3Normalize(targetVec);
	_boneMatrices[ik.nodeIdxes[0]]=LookAtMatrix(originVec, targetVec, XMFLOAT3(0, 1, 0), XMFLOAT3(1, 0, 0));
}

void 
PMDActor::SolveCosineIK(const PMDIK& ik) {
	//�u���v�����߂�
	//�����^�񒆂��u�Ђ��v�ł������ꍇ�ɂ͋����I��X���Ƃ���B

	vector<XMVECTOR> positions;
	std::array<float, 2> edgeLens;

	//IK�`�F�[�����t���Ȃ̂ŁA�t�ɕ��Ԃ悤�ɂ��Ă���
	auto& targetNode = _boneNodeTable[_boneNameArray[ik.targetIdx]];
	positions.emplace_back(XMLoadFloat3(&targetNode.startPos));

	for (auto& chainBoneIdx : ik.nodeIdxes) {
		auto& boneNode = _boneNodeTable[_boneNameArray[chainBoneIdx]];
		positions.emplace_back(XMLoadFloat3(&boneNode.startPos));
	}

	reverse(positions.begin(), positions.end());

	//���̒����𑪂��Ă���
	edgeLens[0] = XMVector3Length(XMVectorSubtract(positions[1], positions[0])).m128_f32[0];
	edgeLens[1] = XMVector3Length(XMVectorSubtract(positions[2], positions[1])).m128_f32[0];

	//�^�[�Q�b�g�ƃ��[�g�m�[�h�����݂̍s��ō��W�ϊ�����
	//���m�[�h�͍������Ɍ������Đ������邽��1�Ԃ����[�g�ɂȂ��Ă���
	auto rootPos = positions[0];

	//������
	positions[0] = XMVector3Transform(positions[0], _boneMatrices[ik.nodeIdxes[1]]);

	//�^�[�Q�b�g
	positions[2] = XMVector3Transform(positions[2], _boneMatrices[ik.targetIdx]);

	//�^�[�Q�b�g�ƃ��[�g�̃x�N�g��������Ă���
	auto linearVec = XMVectorSubtract(positions[2], positions[0]);
	float A = XMVector3Length(linearVec).m128_f32[0];
	float B = edgeLens[0];
	float C = edgeLens[1];

	linearVec = XMVector3Normalize(linearVec);


	//���[�g����^�񒆂ւ̊p�x�v�Z
	float theta1 = acosf((A*A + B * B - C * C) / (2 * A*B));


	//�^�񒆂���^�[�Q�b�g�ւ̊p�x�v�Z
	float theta2 = acosf((B*B + C * C - A * A) / (2 * B*C));

	//�u���v�����߂�
	//�����^�񒆂��u�Ђ��v�ł������ꍇ�ɂ͋����I��X���Ƃ���B
	//���ӓ_�cIK�`�F�[���͍������Ɍ������Ă��琔�����邽��1���������ɋ߂�
	auto mat1 = XMMatrixTranslationFromVector(-positions[0]);
	mat1 *= XMMatrixRotationX(theta1);
	mat1 *= XMMatrixTranslationFromVector(positions[0]);

	auto& pareMat = _boneMatrices[ik.nodeIdxes[1]];

	auto mat2 = XMMatrixTranslationFromVector(-positions[1]);
	mat2 *= XMMatrixRotationX(theta2-XM_PI);
	mat2 *= XMMatrixTranslationFromVector(positions[1]);

	_boneMatrices[ik.nodeIdxes[1]] *= mat1;

	_boneMatrices[ik.nodeIdxes[0]] = mat2 * _boneMatrices[ik.nodeIdxes[1]];// _boneMatrices[ik.nodeIdxes[0]] * mat1;// mat2*mat1;// *_boneMatrices[ik.nodeIdxes[0]];
}

void 
PMDActor::SolveCCDIK(const PMDIK& ik) {
	auto& ikMat = _boneMatrices[ik.boneIdx];
	auto& targetMat = _boneMatrices[ik.targetIdx];
	targetMat *= ikMat;
	for (auto& child : ik.nodeIdxes) {
		_boneMatrices[child] = ikMat;
	}
	auto& boneName= _boneNameArray[ik.targetIdx];
	auto targetBoneNode = _boneNodeAddressArray[ik.targetIdx];
	//XMFLOAT3 ikOriginPos = bone.startPos;//����IK�̌��̍��W
	//XMFLOAT3 ikTargetPos = bone.startPos + offset;//�ړ����IK�̍��W
	//_ikpos = ikTargetPos;//�\���pIK���W�ɑ��

	////�܂���IK�̊Ԃɂ���{�[���̍��W�̈ꎞ�ϐ��z�������āA�l���R�s�[����
	////���R��IK�ċA���閈�ɁA�{�[�����W���ύX����邩��ł���(���̍��W�͕ێ����K�v�Ȃ̂ňꎟ�ϐ��Ɋi�[)
	//std::vector<XMFLOAT3> tmpBonePositions(iklist.ikchainLen);
	//for (int i = 0; i < iklist.ikchainLen; ++i) {
	//	tmpBonePositions[i] = mesh.Bones()[iklist.ikboneIndices[i]].headpos;
	//}

	////�{�[���̍���������(IK����ł������{�[��)����IK�̌��̍��W�ւ̃x�N�g��������Ă���(���쐬�p)
	//XMFLOAT3 ikOriginRootVec = ikOriginPos - tmpBonePositions[iklist.ikchainLen - 1];

	////�{�[���̍���������(IK����ł������{�[��)����ړ���IK���W�ւ̃x�N�g��������Ă���(���쐬�p)
	//XMFLOAT3 ikTargetRootVec = ikTargetPos - tmpBonePositions[iklist.ikchainLen - 1];

	//float ikmaxLen = Length(ikOriginRootVec);
	//if (Length(ikTargetRootVec) > ikmaxLen) {
	//	XMVECTOR vec = XMLoadFloat3(&ikTargetRootVec);
	//	vec = XMVector3ClampLength(vec, 0.1, ikmaxLen);
	//	XMStoreFloat3(&ikTargetRootVec, vec);
	//	ikTargetPos = ikTargetRootVec + tmpBonePositions[iklist.ikchainLen - 1];
	//	_ikpos = ikTargetPos;
	//}
	//if (ikOriginRootVec == ikTargetRootVec) {
	//	return;
	//}

	////IK�ړ��O����ړ���ւ̉�]�x�N�g�����v�Z���Ă���
	//XMMATRIX matIkRot = LookAtMatrix(Normalize(ikOriginRootVec), Normalize(ikTargetRootVec), XMFLOAT3(0, 1, 0), XMFLOAT3(1, 0, 0));

	////�������炪CCD-IK��
	////�z���g�͂����ŃT�C�N���b�N(�J��Ԃ�)��ł����A�܂����ڂ̂��Ƃ���
	////�l���܂��B
	//for (int c = 0; c < iklist.iterations; ++c) {
	//	//���ԃ{�[�����W�␳
	//	for (int i = 0; i < iklist.ikchainLen; ++i) {
	//		int ikboneIdx = iklist.ikboneIndices[i];
	//		PMDMesh::Bone& bone = mesh.Bones()[ikboneIdx];
	//		//�␳���邽�тɃR���g���[���|�C���g�̍��W�͕ς�邪�����ɑ΂���x�N�g�����Čv�Z���Ă����B
	//		XMFLOAT3 originVec = ikOriginPos - tmpBonePositions[i];//���Ƃ̐������IK�Ƃ����̂ڂ�m�[�h�Ńx�N�g���쐬
	//		XMFLOAT3 targetVec = ikTargetPos - tmpBonePositions[i];//�ڕW�n�_�Ƃ����̂ڂ�m�[�h�Ńx�N�g�����쐬
	//		if (originVec == targetVec)return;
	//		//���ꂼ��̃x�N�g����������������ꍇ�͏�����ł��؂�
	//		if (Length(originVec) < 0.0001f || Length(targetVec) < 0.0001f) {
	//			return;
	//		}

	//		//���K�����܂�
	//		originVec = Normalize(originVec);
	//		targetVec = Normalize(targetVec);

	//		//�O�ς��玲���쐬���܂�
	//		XMFLOAT3 axis = Normalize(Cross(originVec, targetVec));

	//		//�����Ђ��n�Ȃ�Ax������]���Ƃ���
	//		if (bone.name.find("�Ђ�") != std::string::npos) {
	//			axis.x = -1;
	//			axis.y = 0;
	//			axis.z = 0;
	//			//���̎���matIkRot�ŉ�]����B
	//			XMVECTOR tmpvec = XMLoadFloat3(&axis);
	//			tmpvec = XMVector3Transform(tmpvec, matIkRot);
	//			XMStoreFloat3(&axis, tmpvec);
	//		}
	//		else {
	//			if (Length(axis) == 0.0f) {
	//				return;//�O�ό��ʂ�0�ɂȂ��Ă�Ȃ�g���܂���
	//			}
	//		}
	//		//�ӂ��̃x�N�g���̊Ԃ̊p�x���v�Z(�����p�x���Z�̂���)
	//		float angle = XMVector3AngleBetweenNormals(XMLoadFloat3(&originVec), XMLoadFloat3(&targetVec)).m128_f32[0];
	//		angle *= 0.5;//���������ɂ��Ă�̉��́c�H

	//		////�p�x������������ꍇ�͏�����ł��؂�
	//		if (abs(angle) == 0.000f) {
	//			return;
	//		}

	//		////�����p�x���v�Z
	//		float strict = iklist.limitAngle * 4;//�����p�x�͎����Ă����p�x�̂S�{
	//		//����ȏ�ɋȂ����Ȃ��悤�ɂ��Ƃ�
	//		angle = min(angle, strict);
	//		angle = max(angle, -strict);

	//		//�{�[���ϊ��s����v�Z
	//		XMMATRIX rotMat = XMMatrixRotationAxis(XMLoadFloat3(&axis), angle);

	//		//�ꎞ�{�[�����W�͎��̃x�N�g������邽�߂ɕK�v�Ȃ̂ōČv�Z����B
	//		XMFLOAT3& tmpbonePosition = tmpBonePositions[i];
	//		XMMATRIX mat = XMMatrixTranslation(-tmpbonePosition.x, -tmpbonePosition.y, -tmpbonePosition.z)*
	//			rotMat*
	//			XMMatrixTranslation(tmpbonePosition.x, tmpbonePosition.y, tmpbonePosition.z);

	//		//�{�[�����_���W���X�V
	//		ikOriginPos = ikOriginPos * mat;
	//		for (int j = 0; j < i; ++j) {
	//			tmpBonePositions[j] = tmpBonePositions[j] * mat;
	//		}

	//		//���ۂɃ{�[������]�����邽�߂̍s������(���_�Ɉړ�����]�����̍��W)
	//		mat = XMMatrixTranslation(-bone.headpos.x, -bone.headpos.y, -bone.headpos.z)*
	//			rotMat*
	//			XMMatrixTranslation(bone.headpos.x, bone.headpos.y, bone.headpos.z);

	//		//�ϊ��s����v�Z(�I�t�Z�b�g���l��)
	//		mesh.BoneMatrixes()[ikboneIdx] = mesh.BoneMatrixes()[ikboneIdx] * mat;
	//	}
	//}
}

float 
PMDActor::GetYFromXOnBezier(float x, const XMFLOAT2& a, const XMFLOAT2& b, uint8_t n) {
	if (a.x == a.y&&b.x == b.y)return x;//�v�Z�s�v
	float t = x;
	const float k0 = 1 + 3 * a.x - 3 * b.x;//t^3�̌W��
	const float k1 = 3 * b.x - 6 * a.x;//t^2�̌W��
	const float k2 = 3 * a.x;//t�̌W��

	//�덷�͈͓̔����ǂ����Ɏg�p����萔
	constexpr float epsilon = 0.0005f;

	for (int i = 0; i < n; ++i) {
		//f(t)���߂܁[��
		auto ft = k0 * t*t*t + k1 * t*t + k2 * t - x;
		//�������ʂ�0�ɋ߂�(�덷�͈͓̔�)�Ȃ�ł��؂�
		if (ft <= epsilon && ft >= -epsilon)break;

		t -= ft / 2;
	}
	//���ɋ��߂���t�͋��߂Ă���̂�y���v�Z����
	auto r = 1 - t;
	return t * t*t + 3 * t*t*r*b.y + 3 * t*r*r*a.y;
}

void* 
PMDActor::Transform::operator new(size_t size) {
	return _aligned_malloc(size, 16);
}

void
PMDActor::RecursiveMatrixMultipy(BoneNode* node, const DirectX::XMMATRIX& mat) {
	//IK�ł��A�e�̉e�����󂯂Ȃ��Ȃ炱���Őe�̍s�����Z���Ȃ��悤�ɂ��Ă���
	if (node->boneType == (uint32_t)BoneType::IK && node->ikParentBone == -1)return;
	if (node->boneType == (uint32_t)::BoneType::IKChild && node->ikParentBone>0) {
		//IK�����������AIK�̃^�[�Q�b�g�{�[���ł���ΐe�̉e�����󂯂Ȃ��悤�ɂ��Ă���
		auto boneIdx = node->ikParentBone;
		auto ikIt = find_if(_ikData.begin(), _ikData.end(), [boneIdx](const PMDIK& ik) {return ik.boneIdx == boneIdx; });
		if (ikIt != _ikData.end()) {
			if (ikIt->targetIdx == node->boneIdx) {
				return;
			}
		}
	}
	_boneMatrices[node->boneIdx] *= mat;
	for (auto& cnode : node->children) {
		RecursiveMatrixMultipy(cnode, _boneMatrices[node->boneIdx]);
	}
}


PMDActor::PMDActor(const char* filepath,PMDRenderer& renderer):
	_renderer(renderer),
	_dx12(renderer._dx12),
	_angle(0.0f)
{
	_transform.world = XMMatrixIdentity();
	LoadPMDFile(filepath);
	CreateTransformView();
	CreateMaterialData();
	CreateMaterialAndTextureView();



	//RecursiveMatrixMultipy(&_boneNodeTable["�Z���^�["], XMMatrixIdentity());
	//XMMatrixRotationQuaternion()
	//copy(_boneMatrices.begin(), _boneMatrices.end(), _mappedMatrices + 1);
}


PMDActor::~PMDActor()
{
}

void 
PMDActor::LoadVMDFile(const char* filepath, const char* name) {
	auto fp = fopen(filepath, "rb");
	fseek(fp, 50, SEEK_SET);//�ŏ���50�o�C�g�͔�΂���OK
	unsigned int keyframeNum = 0;
	fread(&keyframeNum, sizeof(keyframeNum), 1, fp);

	struct VMDKeyFrame {
		char boneName[15]; // �{�[����
		unsigned int frameNo; // �t���[���ԍ�(�Ǎ����͌��݂̃t���[���ʒu��0�Ƃ������Έʒu)
		XMFLOAT3 location; // �ʒu
		XMFLOAT4 quaternion; // Quaternion // ��]
		unsigned char bezier[64]; // [4][4][4]  �x�W�F�⊮�p�����[�^
	};
	vector<VMDKeyFrame> keyframes(keyframeNum);
	for (auto& keyframe : keyframes) {
		fread(keyframe.boneName, sizeof(keyframe.boneName), 1, fp);//�{�[����
		fread(&keyframe.frameNo, sizeof(keyframe.frameNo) +//�t���[���ԍ�
			sizeof(keyframe.location) +//�ʒu(IK�̂Ƃ��Ɏg�p�\��)
			sizeof(keyframe.quaternion) +//�N�I�[�^�j�I��
			sizeof(keyframe.bezier), 1, fp);//��ԃx�W�F�f�[�^
	}

	//VMD�̃L�[�t���[���f�[�^����A���ۂɎg�p����L�[�t���[���e�[�u���֕ϊ�
	for (auto& f : keyframes) {
		_motiondata[f.boneName].emplace_back(KeyFrame(f.frameNo, XMLoadFloat4(&f.quaternion),f.location,
			XMFLOAT2((float)f.bezier[3]/127.0f,(float)f.bezier[7]/127.0f),
			XMFLOAT2((float)f.bezier[11] / 127.0f, (float)f.bezier[15] / 127.0f)));
		_duration = std::max<unsigned int>(_duration, f.frameNo);
	}

	for (auto& motion : _motiondata) {
		sort(motion.second.begin(),motion.second.end(),
			[](const KeyFrame& lval,const KeyFrame& rval){
				return lval.frameNo < rval.frameNo;
			});
	}

	for (auto& bonemotion : _motiondata) {
		auto itBoneNode = _boneNodeTable.find(bonemotion.first);
		if (itBoneNode == _boneNodeTable.end()) {
			continue;
		}
		auto& node = itBoneNode->second;
		auto& pos = node.startPos;
		auto mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z)*
			XMMatrixRotationQuaternion(bonemotion.second[0].quaternion)*
			XMMatrixTranslation(pos.x, pos.y, pos.z);
		_boneMatrices[node.boneIdx] = mat;
	}
	RecursiveMatrixMultipy(&_boneNodeTable["�Z���^�["], XMMatrixIdentity());
	copy(_boneMatrices.begin(), _boneMatrices.end(), _mappedMatrices + 1);

}

void 
PMDActor::PlayAnimation() {
	_startTime = timeGetTime();
}
void 
PMDActor::MotionUpdate() {

	auto elapsedTime = timeGetTime() - _startTime;//�o�ߎ��Ԃ𑪂�
	unsigned int frameNo = 30 * (elapsedTime / 1000.0f);
	if (frameNo > _duration) {
		_startTime = timeGetTime();
		frameNo = 0;
	}

	//�s����N���A(���ĂȂ��ƑO�t���[���̃|�[�Y���d�ˊ|������ă��f��������)
	std::fill(_boneMatrices.begin(), _boneMatrices.end(), XMMatrixIdentity());

	//���[�V�����f�[�^�X�V
	for (auto& bonemotion : _motiondata) {
		auto& boneName = bonemotion.first;
		auto itBoneNode = _boneNodeTable.find(boneName);
		if (itBoneNode == _boneNodeTable.end()) {
			continue;
		}
		auto node = itBoneNode->second;


		//���v������̂�T��
		auto keyframes = bonemotion.second;

		auto rit=find_if(keyframes.rbegin(), keyframes.rend(), [frameNo](const KeyFrame& keyframe) {
			return keyframe.frameNo <= frameNo;
		});
		if (rit == keyframes.rend())continue;//���v������̂��Ȃ���Δ�΂�
		XMMATRIX rotation=XMMatrixIdentity();
		XMVECTOR offset = XMLoadFloat3(&rit->offset); 
		auto it = rit.base();
		if (it != keyframes.end()) {
			auto t = static_cast<float>(frameNo - rit->frameNo) / 
					static_cast<float>(it->frameNo - rit->frameNo);
			t = GetYFromXOnBezier(t, it->p1, it->p2, 12);
			rotation = XMMatrixRotationQuaternion(
						XMQuaternionSlerp(rit->quaternion,it->quaternion,t)
					);
			offset = XMVectorLerp(offset, XMLoadFloat3(&it->offset), t);
		}
		else {
			rotation=XMMatrixRotationQuaternion(rit->quaternion);
		}

		auto& pos = node.startPos;
		auto mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z)*//���_�ɖ߂�
			rotation*//��]
			XMMatrixTranslation(pos.x, pos.y, pos.z);//���̍��W�ɖ߂�
		_boneMatrices[node.boneIdx] =mat*XMMatrixTranslationFromVector(offset);
	}
	RecursiveMatrixMultipy(&_boneNodeTable["�Z���^�["], XMMatrixIdentity());

	IKSolve();

	copy(_boneMatrices.begin(), _boneMatrices.end(), _mappedMatrices + 1);
}

void 
PMDActor::IKSolve() {
	//�܂���IK�̃^�[�Q�b�g�{�[���𓮂���
	for (auto& ik : _ikData) {
		auto childrenNodesCount = ik.nodeIdxes.size();
		switch(childrenNodesCount) {
		case 0://�Ԃ̃{�[������0(���肦�Ȃ�)
			assert(0);
			continue;
		case 1://�Ԃ̃{�[������1�̂Ƃ���LookAt
			SolveLookAt(ik);
			break;
		case 2://�Ԃ̃{�[������2�̂Ƃ��͗]���藝IK
			SolveCosineIK(ik);
			break;
		case 3://3�ȏ�̎���CCD-IK
			SolveCCDIK(ik);
		}
	}
}

HRESULT
PMDActor::LoadPMDFile(const char* path) {
	//PMD�w�b�_�\����
	struct PMDHeader {
		float version; //��F00 00 80 3F == 1.00
		char model_name[20];//���f����
		char comment[256];//���f���R�����g
	};
	char signature[3];
	PMDHeader pmdheader = {};

	string strModelPath = path;

	auto fp = fopen(strModelPath.c_str(), "rb");
	if (fp == nullptr) {
		//�G���[����
		assert(0);
		return ERROR_FILE_NOT_FOUND;
	}
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);

	unsigned int vertNum;//���_��
	fread(&vertNum, sizeof(vertNum), 1, fp);


#pragma pack(1)//��������1�o�C�g�p�b�L���O�c�A���C�����g�͔������Ȃ�
	//PMD�}�e���A���\����
	struct PMDMaterial {
		XMFLOAT3 diffuse; //�f�B�t���[�Y�F
		float alpha; // �f�B�t���[�Y��
		float specularity;//�X�y�L�����̋���(��Z�l)
		XMFLOAT3 specular; //�X�y�L�����F
		XMFLOAT3 ambient; //�A���r�G���g�F
		unsigned char toonIdx; //�g�D�[���ԍ�(��q)
		unsigned char edgeFlg;//�}�e���A�����̗֊s���t���O
		//2�o�C�g�̃p�f�B���O�������I�I
		unsigned int indicesNum; //���̃}�e���A�������蓖����C���f�b�N�X��
		char texFilePath[20]; //�e�N�X�`���t�@�C����(�v���X�A���t�@�c��q)
	};//70�o�C�g�̂͂��c�ł��p�f�B���O���������邽��72�o�C�g
#pragma pack()//1�o�C�g�p�b�L���O����

	constexpr unsigned int pmdvertex_size = 38;//���_1������̃T�C�Y
	std::vector<unsigned char> vertices(vertNum*pmdvertex_size);//�o�b�t�@�m��
	fread(vertices.data(), vertices.size(), 1, fp);//��C�ɓǂݍ���

	unsigned int indicesNum;//�C���f�b�N�X��
	fread(&indicesNum, sizeof(indicesNum), 1, fp);//

	//UPLOAD(�m�ۂ͉\)
	auto result = _dx12.Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertices.size()),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_vb.ReleaseAndGetAddressOf()));

	unsigned char* vertMap = nullptr;
	result = _vb->Map(0, nullptr, (void**)&vertMap);
	std::copy(vertices.begin(), vertices.end(), vertMap);
	_vb->Unmap(0, nullptr);


	_vbView.BufferLocation = _vb->GetGPUVirtualAddress();//�o�b�t�@�̉��z�A�h���X
	_vbView.SizeInBytes = vertices.size();//�S�o�C�g��
	_vbView.StrideInBytes = pmdvertex_size;//1���_������̃o�C�g��

	std::vector<unsigned short> indices(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);//��C�ɓǂݍ���


	//�ݒ�́A�o�b�t�@�̃T�C�Y�ȊO���_�o�b�t�@�̐ݒ���g���܂킵��
	//OK���Ǝv���܂��B
	result = _dx12.Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_ib.ReleaseAndGetAddressOf()));

	//������o�b�t�@�ɃC���f�b�N�X�f�[�^���R�s�[
	unsigned short* mappedIdx = nullptr;
	_ib->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(indices.begin(), indices.end(), mappedIdx);
	_ib->Unmap(0, nullptr);


	//�C���f�b�N�X�o�b�t�@�r���[���쐬
	_ibView.BufferLocation = _ib->GetGPUVirtualAddress();
	_ibView.Format = DXGI_FORMAT_R16_UINT;
	_ibView.SizeInBytes = indices.size() * sizeof(indices[0]);

	unsigned int materialNum;
	fread(&materialNum, sizeof(materialNum), 1, fp);
	_materials.resize(materialNum);
	_textureResources.resize(materialNum);
	_sphResources.resize(materialNum);
	_spaResources.resize(materialNum);
	_toonResources.resize(materialNum);

	std::vector<PMDMaterial> pmdMaterials(materialNum);
	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, fp);
	//�R�s�[
	for (int i = 0; i < pmdMaterials.size(); ++i) {
		_materials[i].indicesNum = pmdMaterials[i].indicesNum;
		_materials[i].material.diffuse = pmdMaterials[i].diffuse;
		_materials[i].material.alpha = pmdMaterials[i].alpha;
		_materials[i].material.specular = pmdMaterials[i].specular;
		_materials[i].material.specularity = pmdMaterials[i].specularity;
		_materials[i].material.ambient = pmdMaterials[i].ambient;
		_materials[i].additional.toonIdx = pmdMaterials[i].toonIdx;
	}

	for (int i = 0; i < pmdMaterials.size(); ++i) {
		//�g�D�[�����\�[�X�̓ǂݍ���
		char toonFilePath[32];
		sprintf(toonFilePath, "toon/toon%02d.bmp", pmdMaterials[i].toonIdx + 1);
		_toonResources[i] = _dx12.GetTextureByPath(toonFilePath);

		if (strlen(pmdMaterials[i].texFilePath) == 0) {
			_textureResources[i] = nullptr;
			continue;
		}

		string texFileName = pmdMaterials[i].texFilePath;
		string sphFileName = "";
		string spaFileName = "";
		if (count(texFileName.begin(), texFileName.end(), '*') > 0) {//�X�v���b�^������
			auto namepair = SplitFileName(texFileName);
			if (GetExtension(namepair.first) == "sph") {
				texFileName = namepair.second;
				sphFileName = namepair.first;
			}
			else if (GetExtension(namepair.first) == "spa") {
				texFileName = namepair.second;
				spaFileName = namepair.first;
			}
			else {
				texFileName = namepair.first;
				if (GetExtension(namepair.second) == "sph") {
					sphFileName = namepair.second;
				}
				else if (GetExtension(namepair.second) == "spa") {
					spaFileName = namepair.second;
				}
			}
		}
		else {
			if (GetExtension(pmdMaterials[i].texFilePath) == "sph") {
				sphFileName = pmdMaterials[i].texFilePath;
				texFileName = "";
			}
			else if (GetExtension(pmdMaterials[i].texFilePath) == "spa") {
				spaFileName = pmdMaterials[i].texFilePath;
				texFileName = "";
			}
			else {
				texFileName = pmdMaterials[i].texFilePath;
			}
		}
		//���f���ƃe�N�X�`���p�X����A�v���P�[�V��������̃e�N�X�`���p�X�𓾂�
		if (texFileName != "") {
			auto texFilePath = GetTexturePathFromModelAndTexPath(strModelPath, texFileName.c_str());
			_textureResources[i] = _dx12.GetTextureByPath(texFilePath.c_str());
		}
		if (sphFileName != "") {
			auto sphFilePath = GetTexturePathFromModelAndTexPath(strModelPath, sphFileName.c_str());
			_sphResources[i] = _dx12.GetTextureByPath(sphFilePath.c_str());
		}
		if (spaFileName != "") {
			auto spaFilePath = GetTexturePathFromModelAndTexPath(strModelPath, spaFileName.c_str());
			_spaResources[i] = _dx12.GetTextureByPath(spaFilePath.c_str());
		}
	}

	unsigned short boneNum = 0;
	fread(&boneNum, sizeof(boneNum), 1, fp);
#pragma pack(1)
	//�ǂݍ��ݗp�{�[���\����
	struct Bone {
		char boneName[20];//�{�[����
		unsigned short parentNo;//�e�{�[���ԍ�
		unsigned short nextNo;//��[�̃{�[���ԍ�
		unsigned char type;//�{�[�����
		unsigned short ikBoneNo;//IK�{�[���ԍ�
		XMFLOAT3 pos;//�{�[���̊�_���W
	};
#pragma pack()
	vector<Bone> pmdBones(boneNum);
	fread(pmdBones.data(), sizeof(Bone), boneNum, fp);
	

	uint16_t ikNum=0;
	fread(&ikNum, sizeof(ikNum), 1, fp);

	_ikData.resize(ikNum);
	for (auto& ik : _ikData) {
		fread(&ik.boneIdx, sizeof(ik.boneIdx), 1, fp);
		fread(&ik.targetIdx, sizeof(ik.targetIdx), 1, fp);
		uint8_t chainLen = 0;
		fread(&chainLen, sizeof(chainLen), 1, fp);
		ik.nodeIdxes.resize(chainLen);
		fread(&ik.iterations, sizeof(ik.iterations), 1, fp);
		fread(&ik.limit, sizeof(ik.limit), 1, fp);
		if (chainLen == 0)continue;//�ԃm�[�h����0�Ȃ�΂����ŏI���
		fread(ik.nodeIdxes.data(), sizeof(ik.nodeIdxes[0]),chainLen, fp);
	}

	fclose(fp);

	//�ǂݍ��݌�̏���

	_boneNameArray.resize(pmdBones.size());
	_boneNodeAddressArray.resize(pmdBones.size());
	//�{�[�����\�z
	//�C���f�b�N�X�Ɩ��O�̑Ή��֌W�\�z�̂��߂Ɍ�Ŏg��
	//�{�[���m�[�h�}�b�v�����
	for (int idx = 0; idx < pmdBones.size(); ++idx) {
		auto& pb = pmdBones[idx];
		auto& node = _boneNodeTable[pb.boneName];
		node.boneIdx = idx;
		node.startPos = pb.pos;
		node.boneType = pb.type;
		node.ikParentBone = pb.ikBoneNo;
		//�C���f�b�N�X���������₷���悤��
		_boneNameArray[idx] = pb.boneName;
		_boneNodeAddressArray[idx] = &node;
	}
	//�c���[�e�q�֌W���\�z����
	for (auto& pb : pmdBones) {
		//�e�C���f�b�N�X���`�F�b�N(���蓾�Ȃ��ԍ��Ȃ��΂�)
		if (pb.parentNo >= pmdBones.size()) {
			continue;
		}
		auto parentName = _boneNameArray[pb.parentNo];
		_boneNodeTable[parentName].children.emplace_back(&_boneNodeTable[pb.boneName]);
	}

	//�{�[���\�z
	_boneMatrices.resize(pmdBones.size());
	//�{�[�������ׂď���������B
	std::fill(_boneMatrices.begin(), _boneMatrices.end(), XMMatrixIdentity());



	//IK�f�o�b�O�p
	auto getNameFromIdx = [&](uint16_t idx)->string {
		auto it = find_if(_boneNodeTable.begin(), _boneNodeTable.end(), [idx](const pair<string, BoneNode>& obj) {
			return obj.second.boneIdx == idx;
		});
		if (it != _boneNodeTable.end()) {
			return it->first;
		}
		else {
			return "";
		}
	};
	for (auto& ik : _ikData) {
		std::ostringstream oss;
		oss << "IK�{�[���ԍ�=" << ik.boneIdx << ":" << getNameFromIdx(ik.boneIdx) << endl;
		for (auto& node : ik.nodeIdxes) {
			oss << "\t�m�[�h�{�[��=" << node << ":" << getNameFromIdx(node)<<endl;
		}
		OutputDebugString(oss.str().c_str());
	}
}

HRESULT 
PMDActor::CreateTransformView() {
	//GPU�o�b�t�@�쐬
	auto buffSize = sizeof(XMMATRIX)*(1 + _boneMatrices.size());
	buffSize = (buffSize + 0xff)&~0xff;

	auto result = _dx12.Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(buffSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_transformBuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}

	//�}�b�v�ƃR�s�[
	result = _transformBuff->Map(0, nullptr, (void**)&_mappedMatrices);
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}
	_mappedMatrices[0] = _transform.world;
	std::copy(_boneMatrices.begin() ,_boneMatrices.end(),_mappedMatrices+1);

	//�r���[�̍쐬
	D3D12_DESCRIPTOR_HEAP_DESC transformDescHeapDesc = {};
	transformDescHeapDesc.NumDescriptors = 1;//�Ƃ肠�������[���h�ЂƂ�
	transformDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	transformDescHeapDesc.NodeMask = 0;

	transformDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//�f�X�N���v�^�q�[�v���
	result = _dx12.Device()->CreateDescriptorHeap(&transformDescHeapDesc, IID_PPV_ARGS(_transformHeap.ReleaseAndGetAddressOf()));//����
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _transformBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = buffSize;
	_dx12.Device()->CreateConstantBufferView(&cbvDesc, _transformHeap->GetCPUDescriptorHandleForHeapStart());

	return S_OK;
}

HRESULT
PMDActor::CreateMaterialData() {
	//�}�e���A���o�b�t�@���쐬
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff)&~0xff;
	auto result = _dx12.Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(materialBuffSize*_materials.size()),//�ܑ̂Ȃ����ǎd���Ȃ��ł���
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_materialBuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}

	//�}�b�v�}�e���A���ɃR�s�[
	char* mapMaterial = nullptr;
	result = _materialBuff->Map(0, nullptr, (void**)&mapMaterial);
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}
	for (auto& m : _materials) {
		*((MaterialForHlsl*)mapMaterial) = m.material;//�f�[�^�R�s�[
		mapMaterial += materialBuffSize;//���̃A���C�����g�ʒu�܂Ői�߂�
	}
	_materialBuff->Unmap(0, nullptr);

	return S_OK;

}


HRESULT 
PMDActor::CreateMaterialAndTextureView() {
	D3D12_DESCRIPTOR_HEAP_DESC materialDescHeapDesc = {};
	materialDescHeapDesc.NumDescriptors = _materials.size() * 5;//�}�e���A�����Ԃ�(�萔1�A�e�N�X�`��3��)
	materialDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	materialDescHeapDesc.NodeMask = 0;

	materialDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//�f�X�N���v�^�q�[�v���
	auto result = _dx12.Device()->CreateDescriptorHeap(&materialDescHeapDesc, IID_PPV_ARGS(_materialHeap.ReleaseAndGetAddressOf()));//����
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff)&~0xff;
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
	matCBVDesc.BufferLocation = _materialBuff->GetGPUVirtualAddress();
	matCBVDesc.SizeInBytes = materialBuffSize;
	
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;//��q
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2D�e�N�X�`��
	srvDesc.Texture2D.MipLevels = 1;//�~�b�v�}�b�v�͎g�p���Ȃ��̂�1
	CD3DX12_CPU_DESCRIPTOR_HANDLE matDescHeapH(_materialHeap->GetCPUDescriptorHandleForHeapStart());
	auto incSize = _dx12.Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (int i = 0; i < _materials.size(); ++i) {
		//�}�e���A���Œ�o�b�t�@�r���[
		_dx12.Device()->CreateConstantBufferView(&matCBVDesc, matDescHeapH);
		matDescHeapH.ptr += incSize;
		matCBVDesc.BufferLocation += materialBuffSize;
		if (_textureResources[i] == nullptr) {
			srvDesc.Format = _renderer._whiteTex->GetDesc().Format;
			_dx12.Device()->CreateShaderResourceView(_renderer._whiteTex.Get(), &srvDesc, matDescHeapH);
		}
		else {
			srvDesc.Format = _textureResources[i]->GetDesc().Format;
			_dx12.Device()->CreateShaderResourceView(_textureResources[i].Get(), &srvDesc, matDescHeapH);
		}
		matDescHeapH.Offset(incSize);

		if (_sphResources[i] == nullptr) {
			srvDesc.Format = _renderer._whiteTex->GetDesc().Format;
			_dx12.Device()->CreateShaderResourceView(_renderer._whiteTex.Get(), &srvDesc, matDescHeapH);
		}
		else {
			srvDesc.Format = _sphResources[i]->GetDesc().Format;
			_dx12.Device()->CreateShaderResourceView(_sphResources[i].Get(), &srvDesc, matDescHeapH);
		}
		matDescHeapH.ptr += incSize;

		if (_spaResources[i] == nullptr) {
			srvDesc.Format = _renderer._blackTex->GetDesc().Format;
			_dx12.Device()->CreateShaderResourceView(_renderer._blackTex.Get(), &srvDesc, matDescHeapH);
		}
		else {
			srvDesc.Format = _spaResources[i]->GetDesc().Format;
			_dx12.Device()->CreateShaderResourceView(_spaResources[i].Get(), &srvDesc, matDescHeapH);
		}
		matDescHeapH.ptr += incSize;


		if (_toonResources[i] == nullptr) {
			srvDesc.Format = _renderer._gradTex->GetDesc().Format;
			_dx12.Device()->CreateShaderResourceView(_renderer._gradTex.Get(), &srvDesc, matDescHeapH);
		}
		else {
			srvDesc.Format = _toonResources[i]->GetDesc().Format;
			_dx12.Device()->CreateShaderResourceView(_toonResources[i].Get(), &srvDesc, matDescHeapH);
		}
		matDescHeapH.ptr += incSize;
	}
}


void 
PMDActor::Update() {
	_angle += 0.001f;
	_mappedMatrices[0] =  XMMatrixRotationY(_angle);
	MotionUpdate();
}
void 
PMDActor::Draw() {
	_dx12.CommandList()->IASetVertexBuffers(0, 1, &_vbView);
	_dx12.CommandList()->IASetIndexBuffer(&_ibView);

	ID3D12DescriptorHeap* transheaps[] = {_transformHeap.Get()};
	_dx12.CommandList()->SetDescriptorHeaps(1, transheaps);
	_dx12.CommandList()->SetGraphicsRootDescriptorTable(1, _transformHeap->GetGPUDescriptorHandleForHeapStart());



	ID3D12DescriptorHeap* mdh[] = { _materialHeap.Get() };
	//�}�e���A��
	_dx12.CommandList()->SetDescriptorHeaps(1, mdh);

	auto materialH = _materialHeap->GetGPUDescriptorHandleForHeapStart();
	unsigned int idxOffset = 0;

	auto cbvsrvIncSize = _dx12.Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
	for (auto& m : _materials) {
		_dx12.CommandList()->SetGraphicsRootDescriptorTable(2, materialH);
		_dx12.CommandList()->DrawIndexedInstanced(m.indicesNum, 1, idxOffset, 0, 0);
		materialH.ptr += cbvsrvIncSize;
		idxOffset += m.indicesNum;
	}

}