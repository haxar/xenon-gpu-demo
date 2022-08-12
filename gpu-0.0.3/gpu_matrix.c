#include "xenos.h"
#include <math.h>
#include <stdio.h>

void M_Load44(int base, eMatrix44 *matrix)
{
	memcpy(x.alu_constants + base * 4, matrix, sizeof(eMatrix44));
	DirtyAluConstant(base, 4);
}

void M_Load43(int base, eMatrix43 *matrix)
{
	memcpy(x.alu_constants + base * 4, matrix, sizeof(eMatrix43));
	DirtyAluConstant(base, 3);
}

eMatrix44 g_ident = {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};

void M_BuildPersp(eMatrix44 *m, float fovy, float aspect, float f, float n)
{
	float cot = 1.0f / tanf(fovy * 0.5F);
	float tmp = 1.0f / (f-n);

	eMatrix44 _m = {
		{cot / aspect, 0, 0, 0},
		{0, cot, 0, 0},
		{0, 0, -n * tmp, 1}, 
		{0, 0, -(f*n) * tmp, 0}};
	memcpy(m, _m, sizeof(_m));
}

void M_Dump(const char *name, eMatrix44 *m)
{
	int i, j;
	printf("-- %s:\n", name);
	for (i=0; i<4; ++i)
	{
		for (j=0; j<4; ++j)
			printf("%3.3f ", (*m)[i][j]);
		printf("\n");
	}
}

eMatrix44 g_proj;

void M_LoadCurrent(void)
{
 	eMatrix44 res, worldview;
	memcpy(worldview, g_ident, sizeof(eMatrix44));
	memcpy(worldview, &matrix_stack[matrix_top], sizeof(eMatrix43));

	multiply_matrix_44(res, g_proj, worldview);

		/* fake fake ... RH -> LH */	
	eMatrix44 tmp;
	int i,j;
	for (i=0; i<4; ++i)
		for (j=0; j<4; ++j)
			tmp[i][j] = res[j][i];
	tmp[3][0] = -tmp[3][0]; 
	tmp[3][1] = -tmp[3][1]; 
	tmp[3][2] = -tmp[3][2]; 
	tmp[3][3] = -tmp[3][3]; 
	M_Load44(0, &tmp);
}

