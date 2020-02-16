Texture2D<float4> texNorm : register(t1);//�@��
//�[�x�l�����p
Texture2D<float> depthTex : register(t4);//�f�v�X

SamplerState smp : register(s0);

cbuffer PostSetting : register(b1) {
	int outline;
	int rimFlg;
	float rimStrength;
	int debugDispFlg;
	int normOutline;
	int directionalLightFlg;
	int aaFlg;
	int bloomFlg;
	int dofFlg;
	int aoFlg;
	int tryCount;
	float aoRadius;
	float4 bloomColor;
	float2 focusPos;
};

//�v���W�F�N�V�����Ƌt�v���W�F�N�V�����̂���
cbuffer sceneBuffer : register(b2) {
	matrix view;//�r���[
	matrix proj;//�v���W�F�N�V����
	matrix invProj;//�t�v���W�F�N�V����
	matrix lightCamera;//���C�g�r���[�v���W�F
	matrix shadow;//�e�s��
	float3 eye;//���_
};

float random(float2 uv) {
	return frac(sin(dot(uv, float2(12.9898, 78.233)))*43758.5453);
}
struct Input {
	float4 pos: SV_POSITION;
	float2 uv:TEXCOORD;
};
//�X�N���[���X�y�[�X�A���r�G���g�I�N���[�W�����p
float SsaoPS(Input input) :SV_TARGET{
	float w,h,mip;
	depthTex.GetDimensions(0,w, h, mip);

	float dx = 1.0f / w, dy = 1.0f / h;
	float dp = depthTex.Sample(smp, input.uv);
	float4 pos = float4(input.uv,dp,1);
	//�@���̍��W�̕���
	pos.xy = pos.xy*float2(2, -2) + float2(-1, 1);
	pos = mul(invProj, pos);
	pos /= pos.w;//�������W�Ŋ���̂�Y�ꂸ��

	float ao = 0.0;//�����������������Z�����
	float accum = 0.0;//�΂ɓ�������(���ׂ�1���������̑��a)
	const int trycnt = tryCount%400;
	const float radius = aoRadius;
	if (dp < 1.0f) {
		float4 norm = texNorm.Sample(smp, input.uv);
		norm.xyz = norm.xyz * 2 - 1;
		for (int i = 0; i < trycnt; ++i) {
			//�A�����_���ȕ����̃x�N�g���ւ����
			float rndX = random(input.uv / 2.0f + float2((float)i*dx, (float)i*dy) / 2.0f);
			float rndY = random(input.uv / 2.0f + float2(rndX, i*dy) / 2.0f);
			float rndZ = random(float2(rndX, rndY));

			//�B�@���x�N�g�����擾���A�����_�������x�N�g���Ɠ��ς����
			//�@���̔��Α��������Ă��甽�]����

			float3 omega = normalize(float3(rndX, rndY, rndZ) * 2 - 1);
			float dt = dot(normalize(norm.xyz), omega);//����=cos��
			if (dt == 0.0f)continue;
			float sgn = sign(dt);//���ς̕����𔻕�
			omega *= sgn;//�@���̔��΋��ɓ����Ă���@�����̔����Ɏ����Ă���
			dt *= sgn;
			//���݂̍��W�Ƀ����_���x�N�g�������Z
			//�C���̍��W���ǂ����ɂ߂荞��ł邩�ǂ����𒲍�����
			float4 rpos = mul(proj, float4(pos.xyz + omega * radius,1));
			rpos /= rpos.w;
			rpos.xy = (rpos.xy + float2(1, -1))*float2(0.5, -0.5);

			ao += step(depthTex.Sample(smp, rpos.xy), rpos.z)*dt;
			accum += dt;
		}
		ao /= accum;
	}
	return 1 - ao;


}