Texture2D<float4> tex:register(t0);//0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> sph:register(t1);//1�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> spa:register(t2);//2�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
SamplerState smp:register(s0);//0�ԃX���b�g�ɐݒ肳�ꂽ�T���v��

//�萔�o�b�t�@0
cbuffer Matrices : register(b0) {
	matrix world;//���[���h�ϊ��s��
	matrix viewproj;//�r���[�v���W�F�N�V�����s��
	float3 eye;
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
	float3 ray:VECTOR;//�x�N�g��
};

Output BasicVS(float4 pos : POSITION , float4 normal : NORMAL, float2 uv : TEXCOORD) {
	Output output;//�s�N�Z���V�F�[�_�֓n���l
	output.svpos = mul(mul(viewproj,world),pos);//�V�F�[�_�ł͗�D��Ȃ̂Œ���
	normal.w = 0;//�����d�v(���s�ړ������𖳌��ɂ���)
	output.normal = mul(world,normal);//�@���ɂ����[���h�ϊ����s��
	output.uv = uv;
	output.ray = normalize(pos.xyz - eye);//�����x�N�g��
	return output;
}

float4 BasicPS(Output input ) : SV_TARGET{
	float3 light = normalize(float3(1,-1,1));
	float3 lightColor = float3(1, 1, 1);
	float diffuseB = dot(-light, input.normal);

	float3 up = float3(0, 1, 0);
	float3 right = normalize(cross(up, input.ray));//�E�x�N�g��

	//���̔��˃x�N�g��
	float3 refLight= normalize(reflect(light, input.normal.xyz));
	float specularB = saturate(dot(refLight, -input.ray));

	//�����̔��˃x�N�g��
	float3 refRay = normalize(reflect(input.ray, input.normal.xyz));
	
	float2 sphereMapUV = float2(dot(input.normal.xyz, right),dot(input.normal.xyz, up));
	sphereMapUV=(sphereMapUV + float2(1, -1))*float2(0.5, -0.5);

	float4 color = tex.Sample(smp, input.uv); //�e�N�X�`���J���[
	return
		saturate(float4(lightColor * diffuseB, 1)
			*diffuse//�f�B�t���[�Y�F
			*color//�e�N�X�`���J���[
			*sph.Sample(smp, sphereMapUV))//�X�t�B�A�}�b�v(��Z)

		+ saturate(float4(pow(specularB, specular.w)
			*specular.rgb
			*lightColor, 1))


		+ spa.Sample(smp, sphereMapUV);//�X�t�B�A�}�b�v(���Z)
		+float4(color*ambient,1);//�A���r�G���g

}