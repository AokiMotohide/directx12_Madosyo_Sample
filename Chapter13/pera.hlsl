
cbuffer Weight : register(b0) {
	float bkweights[8];
};
Texture2D<float4> tex : register(t0);


SamplerState smp : register(s0);

struct Output {
	float4 pos: SV_POSITION;
	float2 uv:TEXCOORD;
};

Output VS(float4 pos:POSITION, float2 uv : TEXCOORD) {
	Output output;
	output.pos = pos;
	output.uv = uv;
	return output;
}
float4 PS(Output input) : SV_TARGET{
	float4 col = tex.Sample(smp,input.uv);
	//float Y = dot(col.rgb, float3(0.299, 0.587, 0.114));
	//return float4(Y, Y, Y, 1);
	//float b = dot(col.rgb, float3(0.2126f, 0.7152f, 0.0722f));
	//return float4(b, b, b, 1);
	float w, h, level;
	tex.GetDimensions(0, w, h, level);
	//float b = dot(float4(1,1,1,1), bkweights1);
	//return float4(b,b,b,1);
	float dx = 1.0f / w;
	float dy = 1.0f / h;
	float4 ret = float4(0, 0, 0, 0);
	ret += bkweights[0] * tex.Sample(smp, input.uv + float2(0, 0));
	for (int i = 1; i < 8; ++i) {
		//ret += bkweights[i>>2][i%4]*tex.Sample(smp, input.uv + float2(i*dx, 0));
		//ret += bkweights[i>>2][i%4] * tex.Sample(smp, input.uv + float2(-i*dy, 0));
		ret += bkweights[i] * tex.Sample(smp, input.uv + float2(i*dx, 0));
		ret += bkweights[i] * tex.Sample(smp, input.uv + float2(-i * dy, 0));
	}
	return float4(ret.rgb,col.a);

	ret += tex.Sample(smp, input.uv + float2(0, -2 * dy))*-1;//��
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0))*-1;//��
	ret += tex.Sample(smp, input.uv) * 4;//����
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0))*-1;//�E
	ret += tex.Sample(smp, input.uv + float2(0, 2 * dy))*-1;//��
	//�����Ŕ��]
	float Y = dot(ret.rgb, float3(0.299, 0.587, 0.114));
	Y = pow(1.0f - Y, 30.0f);
	Y = step(0.2, Y);
	return float4(Y, Y, Y, col.a);
	


	//���̃s�N�Z���𒆐S�ɏc��5���ɂȂ�悤���Z����
	//�ŏ�i
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 1 / 256;
	ret += tex.Sample(smp, input.uv + float2(-1 * dx, 2 * dy)) * 4 / 256;
	ret += tex.Sample(smp, input.uv + float2(0 * dx, 2 * dy)) * 6 / 256;
	ret += tex.Sample(smp, input.uv + float2(1 * dx, 2 * dy)) * 4 / 256;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * 1 / 256;
	//�ЂƂ�i
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 1 * dy)) * 4 / 256;
	ret += tex.Sample(smp, input.uv + float2(-1 * dx, 1 * dy)) * 16 / 256;
	ret += tex.Sample(smp, input.uv + float2(0 * dx, 1 * dy)) * 24 / 256;
	ret += tex.Sample(smp, input.uv + float2(1 * dx, 1 * dy)) * 16 / 256;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 1 * dy)) * 4 / 256;
	//���S��
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0 * dy)) * 6 / 256;
	ret += tex.Sample(smp, input.uv + float2(-1 * dx, 0 * dy)) * 24 / 256;
	ret += tex.Sample(smp, input.uv + float2(0 * dx, 0 * dy)) * 36 / 256;
	ret += tex.Sample(smp, input.uv + float2(1 * dx, 0 * dy)) * 24 / 256;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, 0 * dy)) * 6 / 256;
	//����i
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -1 * dy)) * 4 / 256;
	ret += tex.Sample(smp, input.uv + float2(-1 * dx, -1 * dy)) * 16 / 256;
	ret += tex.Sample(smp, input.uv + float2(0 * dx, -1 * dy)) * 24 / 256;
	ret += tex.Sample(smp, input.uv + float2(1 * dx, -1 * dy)) * 16 / 256;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -1 * dy)) * 4 / 256;
	//�ŉ��i
	ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 1 / 256;
	ret += tex.Sample(smp, input.uv + float2(-1 * dx, -2 * dy)) * 4 / 256;
	ret += tex.Sample(smp, input.uv + float2(0 * dx, -2 * dy)) * 6 / 256;
	ret += tex.Sample(smp, input.uv + float2(1 * dx, -2 * dy)) * 4 / 256;
	ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 1 / 256;

	return ret;
}