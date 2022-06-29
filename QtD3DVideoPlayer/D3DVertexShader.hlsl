struct VSOut {
	float2 tex : TexCoord;
	float4 pos : SV_POSITION;
};


VSOut main_VS(float3 pos : POSITION, float2 tex: TexCoord)
{
	VSOut vsout;
	vsout.pos = float4(pos.x, pos.y, pos.z, 1);
	vsout.tex = tex;
	return vsout;
}
