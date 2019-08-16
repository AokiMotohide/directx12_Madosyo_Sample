Texture2D<float4> tex:register(t0);//0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
SamplerState smp:register(s0);//0�ԃX���b�g�ɐݒ肳�ꂽ�T���v��

//�萔�o�b�t�@0
cbuffer Matrices : register(b0) {
	matrix world;//���[���h�ϊ��s��
	matrix viewproj;//�r���[�v���W�F�N�V�����s��
};
//�萔�o�b�t�@1
//�}�e���A���p
cbuffer Material : register(b1) {
	float4 diffuse;//�f�B�t���[�Y�F
	float4 specular;//�X�y�L����
	float3 ambient;//�A���r�G���g
};

//���_�V�F�[�_���s�N�Z���V�F�[�_�ւ̂����Ɏg�p����
//�\����
struct Output {
	float4 svpos:SV_POSITION;//�V�X�e���p���_���W
	float4 normal:NORMAL;//�@���x�N�g��
	float2 uv:TEXCOORD;//UV�l
};

Output BasicVS(float4 pos : POSITION , float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONE_NO, min16uint weight : WEIGHT) {
	Output output;//�s�N�Z���V�F�[�_�֓n���l
	output.svpos = mul(mul(viewproj,world),pos);//�V�F�[�_�ł͗�D��Ȃ̂Œ���
	normal.w = 0;//�����d�v(���s�ړ������𖳌��ɂ���)
	output.normal = mul(world,normal);//�@���ɂ����[���h�ϊ����s��
	output.uv = uv;
	return output;
}

float4 BasicPS(Output input ) : SV_TARGET{
	float3 light = normalize(float3(1,-1,1));
	float brightness = dot(-light, input.normal);
	return tex.Sample(smp, input.uv)*diffuse;
	return float4(brightness, brightness, brightness, 1)*diffuse;
	//return float4(input.normal.xyz,1);
	//return float4(tex.Sample(smp,input.uv));
}