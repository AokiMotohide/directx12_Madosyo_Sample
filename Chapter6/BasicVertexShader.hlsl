Texture2D<float4> tex:register(t0);//0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
SamplerState smp:register(s0);//0�ԃX���b�g�ɐݒ肳�ꂽ�T���v��

//�萔�o�b�t�@
cbuffer cbuff0 : register(b0) {
	matrix mat;//�ϊ��s��
};

struct Matrix{
	matrix world;
	matrix view;
	matrix proj;
};
ConstantBuffer<Matrix> m: register(b1);

//���_�V�F�[�_���s�N�Z���V�F�[�_�ւ̂����Ɏg�p����
//�\����
struct Output {
	float4 svpos:SV_POSITION;//�V�X�e���p���_���W
	float2 uv:TEXCOORD;//UV�l
};

Output BasicVS(float4 pos : POSITION,float2 uv:TEXCOORD) {
	Output output;//�s�N�Z���V�F�[�_�֓n���l
	output.svpos = mul(mat,pos);
	output.uv = uv;
	return output;
}

float4 BasicPS(Output input ) : SV_TARGET{
	return float4(tex.Sample(smp,input.uv));
}