//SSAO�����̂��߂����̃V�F�[�_
Texture2D<float4> normtex:register(t1);//1�p�X�ڂ̖@���`��
Texture2D<float> depthtex:register(t6);//1�p�X�ڂ̐[�x�e�N�X�`��

SamplerState smp:register(s0);

//�Ԃ��̂�SV_POSITION�����ł͂Ȃ�
struct Output {
	float4 pos: SV_POSITION;
	float2 uv:TEXCOORD;
};

//SSAO(��Z�p�̖��x�̂ݏ���Ԃ���΂悢)
float SsaoPs(Output input) : SV_Target
{
	return 1;
	//float dp = depthtex.Sample(smp, input.uv);//���݂�UV�̐[�x

	//float w, h, miplevels;
	//depthtex.GetDimensions(0, w, h, miplevels);
	//float dx = 1.0 / w;
	//float dy = 1.0 / h;

	////SSAO
	////���̍��W�𕜌�����
	//float4 respos = mul(invproj, float4(input.uv*float2(2, -2) + float2(-1, 1), dp, 1));
	//respos.xyz = respos.xyz / respos.w;
	//float div = 0.0f;
	//float ao = 0.0f;
	//float3 norm = (normtex.Sample(smp, input.uv).xyz * 2) - 1;
	//const int trycnt = 256;
	//const float radius = 1.0f;
	//if (dp < 1.0f) {
	//	for (int i = 0; i < trycnt; ++i) {
	//		float rnd1 = random(float2(i*dx, i*dy)) * 2 - 1;
	//		float rnd2 = random(float2(i*dx + rnd1, i*dy)) * 2 - 1;
	//		float rnd3 = random(float2(i*dx + rnd2, i*dy + rnd1)) * 2 - 1;
	//		float3 omega = norm.xyz + normalize(float3(rnd1,rnd2,rnd3));
	//		omega = normalize(omega);
	//		//�����̌��ʖ@���̔��Α��Ɍ����Ă��甽�]����
	//		float dt = dot(norm, omega);
	//		float sgn = sign(dt);
	//		omega *= sign(dt);
	//		//���ʂ̍��W���Ăюˉe�ϊ�����
	//		float4 rpos = mul(proj, float4(respos.xyz + omega * radius, 1));
	//		rpos.xyz /= rpos.w;
	//		dt *= sgn;
	//		div += dt;
	//		//�v�Z���ʂ����݂̏ꏊ�̐[�x��艜�ɓ����Ă�Ȃ�Օ�����Ă���Ƃ������Ȃ̂ŉ��Z
	//		ao += step(depthtex.Sample(smp, (rpos.xy + float2(1, -1))*float2(0.5, -0.5)), rpos.z)*dt;
	//	}
	//	ao /= div;
	//}
	//return 1.0f - ao;
}
