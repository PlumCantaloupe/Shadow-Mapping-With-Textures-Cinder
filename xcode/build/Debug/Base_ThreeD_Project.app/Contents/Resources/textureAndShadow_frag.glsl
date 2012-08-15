#version 120

uniform sampler2DShadow		depthTexture;
uniform sampler2D			texture;

varying vec3				N, V, L;
varying vec4				q;

void main()
{
    vec3 normal = normalize( N );
	vec3 R = -normalize( reflect( L, normal ) );
	
	vec4 ambient = gl_FrontLightProduct[0].ambient;
	vec4 diffuse = texture2D( texture, gl_TexCoord[0].st );
	vec4 specular = gl_FrontLightProduct[0].specular * pow(max(dot(R, V), 0.0), gl_FrontMaterial.shininess);
	
	vec3 coord  = 0.5 * (q.xyz / q.w + 1.0);
		
	float shadow = shadow2D( depthTexture, coord ).r;
	
	gl_FragColor = ((0.8 * shadow) * diffuse) + ambient + diffuse * 0.3;
}