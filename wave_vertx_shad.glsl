#version 420 core
#define MAX_WAVE_NUMBER 50
#define M_PI 3.14159265

layout(location = 0) in vec3 v_pos;

uniform float time;
uniform int WaveNumber;
uniform float GlobalSteepness;
uniform vec3 LightPosition_worldspace;
uniform vec3 EyePosition;

struct WavePar
{
  vec2 WaveDir;
  float WaveLength;
  float Speed;
  float KAmpOverLen;
  float Phase;
};


layout(std140, binding = 1) uniform WaveVars
{
  WavePar waves[MAX_WAVE_NUMBER];
};


layout(std140, binding = 0) uniform MVP
{
  mat4 model;
  mat4 view;
  mat4 projection;
};



out VS_OUT
{
  vec3 position_worldspace;
  vec3 normal_cameraspace;
  vec3 eyeDirection_cameraspace;
  vec3 lightDirection_cameraspace;
}vs_out;

void main()
{
  vec3 pos_sum = vec3(0);
  vec3 normal_sum = vec3(0);
  for(int i = 0; i < WaveNumber; i++)
  {   
    float Amplitude = waves[i].WaveLength * waves[i].KAmpOverLen;
    float Omega = 2*M_PI / waves[i].WaveLength;
    float Phase = waves[i].Speed * Omega;
    float Steepness = GlobalSteepness /(Omega * Amplitude * WaveNumber);
    float CosTerm = cos(Omega * dot(waves[i].WaveDir, v_pos.xz) + waves[i].Phase * time);
    float SinTerm = sin(Omega * dot(waves[i].WaveDir, v_pos.xz) + waves[i].Phase * time);

	// Compute here Normal
    vec3 smallNormal;
    smallNormal.x = waves[i].WaveDir.x * Omega * Amplitude * CosTerm;
    smallNormal.z = waves[i].WaveDir.y * Omega * Amplitude * CosTerm;
    smallNormal.y = Steepness * Omega * Amplitude * SinTerm;

    normal_sum += smallNormal;

    // Compute here  Position
    vec3 smallPos;
    smallPos.x = Steepness * Amplitude * waves[i].WaveDir.x * CosTerm;
    smallPos.z = Steepness * Amplitude * waves[i].WaveDir.y * CosTerm;
    smallPos.y = Amplitude * sin(Omega * dot(waves[i].WaveDir, v_pos.xz) + waves[i].Phase * time);

    pos_sum += smallPos;

    
  }
  vec3 pos_res;
  pos_res.x = v_pos.x + pos_sum.x;
  pos_res.z = v_pos.z + pos_sum.z;
  pos_res.y = pos_sum.y;
 

  normal_sum.x = -normal_sum.x;
  normal_sum.z = -normal_sum.z;
  normal_sum.y = 1 - normal_sum.y;
  normal_sum = normalize(normal_sum);
  if ((v_pos.x>15 && v_pos.x<25 && v_pos.z>20 && v_pos.z<30) ||(v_pos.x>25 && v_pos.x<32 && v_pos.z>5 && v_pos.z<12) )
	{ gl_Position =  projection * view * model * vec4(v_pos, 1);}

  else 
	{ gl_Position = projection * view * model * vec4(pos_res, 1);

  vs_out.position_worldspace = (model * vec4(pos_res, 1)).xyz;
  vec3 v_pos_camspace = (view * model * vec4(pos_res, 1)).xyz;
  vs_out.eyeDirection_cameraspace = EyePosition - v_pos_camspace;
  vec3 light_pos_camspace = (view * vec4(LightPosition_worldspace, 1)).xyz;
  vs_out.lightDirection_cameraspace = light_pos_camspace + vs_out.eyeDirection_cameraspace;
  vs_out.normal_cameraspace = (view * model * vec4(normal_sum,0)).xyz;}
}
