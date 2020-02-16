SamplerState smp : register(s0);
SamplerState clutSmp : register(s1);
SamplerComparisonState shadowSmp : register(s2);

//�}�e���A���p�X���b�g
cbuffer materialBuffer : register(b0) {
	float4 diffuse;
	float power;
	float3 specular;
	float3 ambient;
};
//�}�e���A���p
Texture2D<float4> tex : register(t0);//�ʏ�e�N�X�`��
Texture2D<float4> sph : register(t1);//�X�t�B�A�}�b�v(��Z)
Texture2D<float4> spa : register(t2);//�X�t�B�A�}�b�v(���Z)
Texture2D<float4> toon : register(t3);//�g�D�[���e�N�X�`��

//�V���h�E�}�b�v�p���C�g�[�x�e�N�X�`��
Texture2D<float> lightDepthTex : register(t4);

//�V�[���Ǘ��p�X���b�g
cbuffer sceneBuffer : register(b1) {
	matrix view;//�r���[
	matrix proj;//�v���W�F�N�V����
	matrix invproj;//�v���W�F�N�V����
	matrix lightCamera;//���C�g�r���[�v���W�F
	matrix shadow;//�e�s��
	float4 lightVec;//���C�g�x�N�g��
	float3 eye;//���_
	bool isSelfShadow;//�V���h�E�}�b�v�t���O
};

//�A�N�^�[���W�ϊ��p�X���b�g
cbuffer transBuffer : register(b2) {
	matrix world;
}

//�{�[���s��z��
cbuffer transBuffer : register(b3) {
	matrix bones[512];
}


//�Ԃ��̂�SV_POSITION�����ł͂Ȃ�
struct Output {
	float4 svpos : SV_POSITION;
	float4 pos : POSITION;
	float4 tpos : TPOS;
	float4 normal : NORMAL;
	float2 uv : TEXCOORD;
	float instNo : INSTNO;
};

struct PixelOutput {
	float4 col:SV_TARGET0;//�ʏ�̃����_�����O����
	float4 normal:SV_TARGET1;//�@��
	float4 highLum:SV_TARGET2;//���P�x(High Luminance)
};

struct PrimitiveOutput {
	float4 svpos:SV_POSITION;
	float4 tpos : TPOS;
	float4 normal:NORMAL;
};

PrimitiveOutput PrimitiveVS(float4 pos:POSITION, float4 normal : NORMAL) {
	PrimitiveOutput output;
	output.svpos = mul(proj, mul(view, pos));
	output.tpos = mul(lightCamera, pos);
	output.normal = normal;
	return output;
}
float4 PrimitivePS(PrimitiveOutput input) : SV_TARGET{
	float3 light = normalize(float3(1,-1,1));
	float bright = dot(input.normal, -light);

	float shadowWeight = 1.0f;
	float3 posFromLightVP = input.tpos.xyz / input.tpos.w;
	float2 shadowUV = (input.tpos.xy / input.tpos.w + float2(1, -1))*float2(0.5, -0.5);
	float depthFromLight = lightDepthTex.SampleCmpLevelZero(
		shadowSmp,
		shadowUV,
		posFromLightVP.z - 0.005f);
	shadowWeight = lerp(0.5f, 1.0f, depthFromLight);

	float b = bright*shadowWeight;

	return float4(b,b,b,1);

}

//���_�V�F�[�_(���_��񂩂�K�v�Ȃ��̂����̐l�֓n��)
//�p�C�v���C���ɓ����邽�߂ɂ�SV_POSITION���K�v
Output VS(float4 pos:POSITION,float4 normal:NORMAL,float2 uv:TEXCOORD,min16uint2 boneno:BONENO,min16uint weight:WEIGHT,uint instNo:SV_InstanceID) {
	//1280,720�𒼂Ŏg���č\��Ȃ��B
	Output output;
	float fWeight = float(weight) / 100.0f;
	matrix conBone = bones[boneno.x]*fWeight + 
						bones[boneno.y]*(1.0f - fWeight);

	output.pos = mul(world, 
						mul(conBone,pos)
					);
	output.instNo = (float)instNo;
	output.svpos = mul(proj,mul(view, output.pos));
	output.tpos = mul(lightCamera, output.pos);
//	output.tpos.w = 1;
	output.uv = uv;
	normal.w = 0;
	output.normal = mul(world,mul(conBone,normal));
	return output;
}


//�s�N�Z���V�F�[�_
PixelOutput PS(Output input) {


	float3 eyeray = normalize(input.pos-eye);
	float3 light = normalize(lightVec);
	float3 rlight = reflect(light, input.normal);
		
	//�X�y�L�����P�x
	float p = saturate(dot(rlight, -eyeray));

	//MSDN��pow�̃h�L�������g�ɂ���
	//p=0��������p==0&&power==0�̂Ƃ�NAN�̉\����
	//���邽�߁A�O�̂��߈ȉ��̂悤�ȃR�[�h�ɂ��Ă���
	//https://docs.microsoft.com/ja-jp/windows/win32/direct3dhlsl/dx-graphics-hlsl-pow
	float specB = 0;
	if (p > 0 && power > 0) {
		specB=pow(p, power);
	}


	float4 texCol = tex.Sample(smp, input.uv);
	float2 spUV = (input.normal.xy
		*float2(1, -1) //�܂��㉺�����Ђ����肩����
		+ float2(1, 1)//(1,1)�𑫂���-1�`1��0�`2�ɂ���
		) / 2;
	float4 sphCol = sph.Sample(smp, spUV);
	float4 spaCol = spa.Sample(smp, spUV);

	//�f�B�t���[�Y���邳		
	float diffB = dot(-light, input.normal);
	float4 toonCol = toon.Sample(clutSmp, float2(0, 1 - diffB));

	
	float4 ret = float4((spaCol + sphCol * texCol * toonCol*diffuse).rgb,diffuse.a)
		+ float4(specular*specB, 1);
	
	float shadowWeight = 1.0f;
	if (isSelfShadow) {
		float3 posFromLightVP = input.tpos.xyz / input.tpos.w;
		float2 shadowUV = (posFromLightVP + float2(1, -1))*float2(0.5, -0.5);
		float depthFromLight = lightDepthTex.SampleCmp(
			shadowSmp,
			shadowUV,
			posFromLightVP.z - 0.005f);
		shadowWeight = lerp(0.5f, 1.0f, depthFromLight);
	}
	PixelOutput output;
	output.col = float4(ret.rgb*shadowWeight, ret.a);
	output.normal.rgb = float3((input.normal.xyz + 1.0f) / 2.0f);
	output.normal.a = 1;
	float y = dot(float3(0.299f, 0.587f, 0.114f), output.col);
	output.highLum = y>0.995f? output.col :0.0;
	output.highLum.a = 1.0f;
	return output;
}

//�e�p���_���W�ϊ�
float4 
shadowVS(float4 pos:POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONENO, min16uint weight : WEIGHT) :SV_POSITION{
	float fWeight = float(weight) / 100.0f;
	matrix conBone = bones[boneno.x] * fWeight +
						bones[boneno.y] * (1.0f - fWeight);

	pos = mul(world, mul(conBone, pos));
	return  mul(lightCamera, pos);
}


