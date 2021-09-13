#version 450

layout (location = 0) in vec3 inPos;

layout(push_constant) uniform PushConsts
{
	vec4 position;
} pushConsts;

out gl_PerVertex 
{
    vec4 gl_Position;   
};
 
void main()
{
	gl_Position =  pushConsts.position * vec4(inPos, 1.0);
}