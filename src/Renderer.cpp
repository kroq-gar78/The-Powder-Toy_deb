/*
 * Renderer.cpp
 *
 *  Created on: Jan 7, 2012
 *      Author: Simon
 */

#include <cmath>
#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include "Config.h"
#include "Renderer.h"
#include "Graphics.h"
#include "simulation/Elements.h"
#include "simulation/ElementGraphics.h"
#include "simulation/Air.h"
extern "C"
{
#include "hmap.h"
#ifdef OGLR
#include "Shaders.h"
#endif
}


void Renderer::clearScreen(float alpha)
{
#ifdef OGLR
	if(alpha > 0.999f)
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, partsFbo);
		glClear(GL_COLOR_BUFFER_BIT);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
    else
    {
		glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
		glColor4f(1.0f, 1.0f, 1.0f, alpha);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, partsFbo);
		glBegin(GL_QUADS);
		glVertex2f(0, 0);
		glVertex2f(XRES, 0);
		glVertex2f(XRES, YRES);
		glVertex2f(0, YRES);
		glEnd();
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlendEquation(GL_FUNC_ADD);
    }
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
#endif
}
#ifdef OGLR
void Renderer::checkShader(GLuint shader, char * shname)
{
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		char errorBuf[ GL_INFO_LOG_LENGTH];
		int errLen;
		glGetShaderInfoLog(shader, GL_INFO_LOG_LENGTH, &errLen, errorBuf);
		fprintf(stderr, "Failed to compile %s shader:\n%s\n", shname, errorBuf);
		exit(1);
	}
}
void Renderer::checkProgram(GLuint program, char * progname)
{
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		char errorBuf[ GL_INFO_LOG_LENGTH];
		int errLen;
		glGetShaderInfoLog(program, GL_INFO_LOG_LENGTH, &errLen, errorBuf);
		fprintf(stderr, "Failed to link %s program:\n%s\n", progname, errorBuf);
		exit(1);
	}
}
void Renderer::loadShaders()
{
	GLuint vertexShader, fragmentShader;

	//Particle texture
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource( vertexShader, 1, &fireVertex, NULL);
	glShaderSource( fragmentShader, 1, &fireFragment, NULL);

	glCompileShader( vertexShader );
	checkShader(vertexShader, "FV");
	glCompileShader( fragmentShader );
	checkShader(fragmentShader, "FF");

	fireProg = glCreateProgram();
	glAttachShader( fireProg, vertexShader );
	glAttachShader( fireProg, fragmentShader );
	glLinkProgram( fireProg );
	checkProgram(fireProg, "F");

	//Lensing
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource( vertexShader, 1, &lensVertex, NULL);
	glShaderSource( fragmentShader, 1, &lensFragment, NULL);

	glCompileShader( vertexShader );
	checkShader(vertexShader, "LV");
	glCompileShader( fragmentShader );
	checkShader(fragmentShader, "LF");

	lensProg = glCreateProgram();
	glAttachShader( lensProg, vertexShader );
	glAttachShader( lensProg, fragmentShader );
	glLinkProgram( lensProg );
	checkProgram(lensProg, "L");

	//Air Velocity
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource( vertexShader, 1, &airVVertex, NULL);
	glShaderSource( fragmentShader, 1, &airVFragment, NULL);

	glCompileShader( vertexShader );
	checkShader(vertexShader, "AVX");
	glCompileShader( fragmentShader );
	checkShader(fragmentShader, "AVF");

	airProg_Velocity = glCreateProgram();
	glAttachShader( airProg_Velocity, vertexShader );
	glAttachShader( airProg_Velocity, fragmentShader );
	glLinkProgram( airProg_Velocity );
	checkProgram(airProg_Velocity, "AV");

	//Air Pressure
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource( vertexShader, 1, &airPVertex, NULL);
	glShaderSource( fragmentShader, 1, &airPFragment, NULL);

	glCompileShader( vertexShader );
	checkShader(vertexShader, "APV");
	glCompileShader( fragmentShader );
	checkShader(fragmentShader, "APF");

	airProg_Pressure = glCreateProgram();
	glAttachShader( airProg_Pressure, vertexShader );
	glAttachShader( airProg_Pressure, fragmentShader );
	glLinkProgram( airProg_Pressure );
	checkProgram(airProg_Pressure, "AP");

	//Air cracker
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource( vertexShader, 1, &airCVertex, NULL);
	glShaderSource( fragmentShader, 1, &airCFragment, NULL);

	glCompileShader( vertexShader );
	checkShader(vertexShader, "ACV");
	glCompileShader( fragmentShader );
	checkShader(fragmentShader, "ACF");

	airProg_Cracker = glCreateProgram();
	glAttachShader( airProg_Cracker, vertexShader );
	glAttachShader( airProg_Cracker, fragmentShader );
	glLinkProgram( airProg_Cracker );
	checkProgram(airProg_Cracker, "AC");
}
#endif

void Renderer::FinaliseParts()
{
#ifdef OGLR
	glEnable( GL_TEXTURE_2D );
	if(display_mode & DISPLAY_WARP)
	{
		float xres = XRES, yres = YRES;
		glUseProgram(lensProg);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, partsFboTex);
		glUniform1i(glGetUniformLocation(lensProg, "pTex"), 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, partsTFX);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, XRES/CELL, YRES/CELL, GL_RED, GL_FLOAT, sim->gravx);
		glUniform1i(glGetUniformLocation(lensProg, "tfX"), 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, partsTFY);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, XRES/CELL, YRES/CELL, GL_GREEN, GL_FLOAT, sim->gravy);
		glUniform1i(glGetUniformLocation(lensProg, "tfY"), 2);
		glActiveTexture(GL_TEXTURE0);
		glUniform1fv(glGetUniformLocation(lensProg, "xres"), 1, &xres);
		glUniform1fv(glGetUniformLocation(lensProg, "yres"), 1, &yres);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, partsFboTex);
		glBlendFunc(GL_ONE, GL_ONE);
	}

	int sdl_scale = 1;
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);
	glTexCoord2d(1, 0);
	//glVertex3f(XRES*sdl_scale, (YRES+MENUSIZE)*sdl_scale, 1.0);
	glVertex3f(XRES*sdl_scale, YRES*sdl_scale, 1.0);
	glTexCoord2d(0, 0);
	//glVertex3f(0, (YRES+MENUSIZE)*sdl_scale, 1.0);
	glVertex3f(0, YRES*sdl_scale, 1.0);
	glTexCoord2d(0, 1);
	//glVertex3f(0, MENUSIZE*sdl_scale, 1.0);
	glVertex3f(0, 0, 1.0);
	glTexCoord2d(1, 1);
	//glVertex3f(XRES*sdl_scale, MENUSIZE*sdl_scale, 1.0);
	glVertex3f(XRES*sdl_scale, 0, 1.0);
	glEnd();

	if(display_mode & DISPLAY_WARP)
	{
		glUseProgram(0);

	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable( GL_TEXTURE_2D );
#endif
}

void Renderer::RenderZoom()
{
	if(!zoomEnabled)
		return;
	#ifdef OGLR
		int sdl_scale = 1;
		int origBlendSrc, origBlendDst;
		float zcx1, zcx0, zcy1, zcy0, yfactor, xfactor, i; //X-Factor is shit, btw
		xfactor = 1.0f/(float)XRES;
		yfactor = 1.0f/(float)YRES;
		yfactor*=-1.0f;

		zcx1 = (zoomScopePosition.X)*xfactor;
		zcx0 = (zoomScopePosition.X+zoomScopeSize)*xfactor;
		zcy1 = (zoomScopePosition.Y-1)*yfactor;
		zcy0 = ((zoomScopePosition.Y-1+zoomScopeSize))*yfactor;

		glGetIntegerv(GL_BLEND_SRC, &origBlendSrc);
		glGetIntegerv(GL_BLEND_DST, &origBlendDst);
		glBlendFunc(GL_ONE, GL_ZERO);

		glEnable( GL_TEXTURE_2D );
		//glReadBuffer(GL_AUX0);
		glBindTexture(GL_TEXTURE_2D, partsFboTex);

		//Draw zoomed texture
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glBegin(GL_QUADS);
		glTexCoord2d(zcx1, zcy1);
		glVertex2i(zoomWindowPosition.X, zoomWindowPosition.Y);
		glTexCoord2d(zcx0, zcy1);
		glVertex2i(zoomWindowPosition.X+(zoomScopeSize*ZFACTOR), zoomWindowPosition.Y);
		glTexCoord2d(zcx0, zcy0);
		glVertex2i(zoomWindowPosition.X+(zoomScopeSize*ZFACTOR), zoomWindowPosition.Y+(zoomScopeSize*ZFACTOR));
		glTexCoord2d(zcx1, zcy0);
		glVertex2i(zoomWindowPosition.X, zoomWindowPosition.Y+(zoomScopeSize*ZFACTOR));
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable( GL_TEXTURE_2D );

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//Lines to make the pixels stand out
		glLineWidth(sdl_scale);
		//glEnable(GL_LINE_SMOOTH);
		glBegin(GL_LINES);
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
		for(i = 0; i < zoomScopeSize; i++)
		{
			//Across
			glVertex2i(zoomWindowPosition.X, zoomWindowPosition.Y+(i*ZFACTOR));
			glVertex2i(zoomWindowPosition.X+(zoomScopeSize*ZFACTOR), zoomWindowPosition.Y+(i*ZFACTOR));

			//Down
			glVertex2i(zoomWindowPosition.X+(i*ZFACTOR), zoomWindowPosition.Y);
			glVertex2i(zoomWindowPosition.X+(i*ZFACTOR), zoomWindowPosition.Y+(zoomScopeSize*ZFACTOR));
		}
		glEnd();

		//Draw zoom window border
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glBegin(GL_LINE_LOOP);
		glVertex2i(zoomWindowPosition.X, zoomWindowPosition.Y);
		glVertex2i(zoomWindowPosition.X+(zoomScopeSize*ZFACTOR), zoomWindowPosition.Y);
		glVertex2i(zoomWindowPosition.X+(zoomScopeSize*ZFACTOR), zoomWindowPosition.Y+(zoomScopeSize*ZFACTOR));
		glVertex2i(zoomWindowPosition.X, zoomWindowPosition.Y+(zoomScopeSize*ZFACTOR));
		glEnd();
		//glDisable(GL_LINE_SMOOTH);

		if(zoomEnabled)
		{
			glEnable(GL_COLOR_LOGIC_OP);
			//glEnable(GL_LINE_SMOOTH);
			glLogicOp(GL_XOR);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glBegin(GL_LINE_LOOP);
			glVertex2i(zoomScopePosition.X, zoomScopePosition.Y);
			glVertex2i(zoomScopePosition.X+zoomScopeSize, zoomScopePosition.Y);
			glVertex2i(zoomScopePosition.X+zoomScopeSize, zoomScopePosition.Y+zoomScopeSize);
			glVertex2i(zoomScopePosition.X, zoomScopePosition.Y+zoomScopeSize);
			/*glVertex3i((zoomScopePosition.X-1)*sdl_scale, (YRES+MENUSIZE-(zoomScopePosition.Y-1))*sdl_scale, 0);
			glVertex3i((zoomScopePosition.X-1)*sdl_scale, (YRES+MENUSIZE-(zoomScopePosition.Y+zoomScopeSize))*sdl_scale, 0);
			glVertex3i((zoomScopePosition.X+zoomScopeSize)*sdl_scale, (YRES+MENUSIZE-(zoomScopePosition.Y+zoomScopeSize))*sdl_scale, 0);
			glVertex3i((zoomScopePosition.X+zoomScopeSize)*sdl_scale, (YRES+MENUSIZE-(zoomScopePosition.Y-1))*sdl_scale, 0);
			glVertex3i((zoomScopePosition.X-1)*sdl_scale, (YRES+MENUSIZE-(zoomScopePosition.Y-1))*sdl_scale, 0);*/
			glEnd();
			glDisable(GL_COLOR_LOGIC_OP);
		}
		glLineWidth(1);
		glBlendFunc(origBlendSrc, origBlendDst);
	#else
		int x, y, i, j;
		pixel pix;
		pixel * img = g->vid;
		g->drawrect(zoomWindowPosition.X-2, zoomWindowPosition.Y-2, zoomScopeSize*ZFACTOR+2, zoomScopeSize*ZFACTOR+2, 192, 192, 192, 255);
		g->drawrect(zoomWindowPosition.X-1, zoomWindowPosition.Y-1, zoomScopeSize*ZFACTOR, zoomScopeSize*ZFACTOR, 0, 0, 0, 255);
		g->clearrect(zoomWindowPosition.X, zoomWindowPosition.Y, zoomScopeSize*ZFACTOR, zoomScopeSize*ZFACTOR);
		for (j=0; j<zoomScopeSize; j++)
			for (i=0; i<zoomScopeSize; i++)
			{
				pix = img[(j+zoomScopePosition.Y)*(XRES+BARSIZE)+(i+zoomScopePosition.X)];
				for (y=0; y<ZFACTOR-1; y++)
					for (x=0; x<ZFACTOR-1; x++)
						img[(j*ZFACTOR+y+zoomWindowPosition.Y)*(XRES+BARSIZE)+(i*ZFACTOR+x+zoomWindowPosition.X)] = pix;
			}
		if (zoomEnabled)
		{
			for (j=-1; j<=zoomScopeSize; j++)
			{
				g->xor_pixel(zoomScopePosition.X+j, zoomScopePosition.Y-1);
				g->xor_pixel(zoomScopePosition.X+j, zoomScopePosition.Y+zoomScopeSize);
			}
			for (j=0; j<zoomScopeSize; j++)
			{
				g->xor_pixel(zoomScopePosition.X-1, zoomScopePosition.Y+j);
				g->xor_pixel(zoomScopePosition.X+zoomScopeSize, zoomScopePosition.Y+j);
			}
		}
	#endif
}

void Renderer::DrawWalls()
{
#ifndef OGLR
	int x, y, i, j, cr, cg, cb;
	unsigned char wt;
	pixel pc;
	pixel gc;
	unsigned char (*bmap)[XRES/CELL] = sim->bmap;
	unsigned char (*emap)[XRES/CELL] = sim->emap;
	wall_type *wtypes = sim->wtypes;
	pixel * vid = g->vid;

	for (y=0; y<YRES/CELL; y++)
		for (x=0; x<XRES/CELL; x++)
			if (bmap[y][x])
			{
				wt = bmap[y][x];
				if (wt<0 || wt>=UI_WALLCOUNT)
					continue;
				pc = wtypes[wt].colour;
				gc = wtypes[wt].eglow;

				// standard wall patterns
				if (wtypes[wt].drawstyle==1)
				{
					for (j=0; j<CELL; j+=2)
						for (i=(j>>1)&1; i<CELL; i+=2)
							vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = pc;
				}
				else if (wtypes[wt].drawstyle==2)
				{
					for (j=0; j<CELL; j+=2)
						for (i=0; i<CELL; i+=2)
							vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = pc;
				}
				else if (wtypes[wt].drawstyle==3)
				{
					for (j=0; j<CELL; j++)
						for (i=0; i<CELL; i++)
							vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = pc;
				}
				else if (wtypes[wt].drawstyle==4)
				{
					for (j=0; j<CELL; j++)
						for (i=0; i<CELL; i++)
							if(i == j)
								vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = pc;
							else if  (i == j+1 || (i == 0 && j == CELL-1))
								vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = gc;
							else
								vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = PIXPACK(0x202020);
				}

				// special rendering for some walls
				if (wt==WL_EWALL)
				{
					if (emap[y][x])
					{
						for (j=0; j<CELL; j++)
							for (i=0; i<CELL; i++)
								if (i&j&1)
									vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = pc;
					}
					else
					{
						for (j=0; j<CELL; j++)
							for (i=0; i<CELL; i++)
								if (!(i&j&1))
									vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = pc;
					}
				}
				else if (wt==WL_WALLELEC)
				{
					for (j=0; j<CELL; j++)
						for (i=0; i<CELL; i++)
						{
							if (!((y*CELL+j)%2) && !((x*CELL+i)%2))
								vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = pc;
							else
								vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = PIXPACK(0x808080);
						}
				}
				else if (wt==WL_EHOLE)
				{
					if (emap[y][x])
					{
						for (j=0; j<CELL; j++)
							for (i=0; i<CELL; i++)
								vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = PIXPACK(0x242424);
						for (j=0; j<CELL; j+=2)
							for (i=0; i<CELL; i+=2)
								vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = PIXPACK(0x000000);
					}
					else
					{
						for (j=0; j<CELL; j+=2)
							for (i=0; i<CELL; i+=2)
								vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = PIXPACK(0x242424);
					}
				}
				if (wtypes[wt].eglow && emap[y][x])
				{
					// glow if electrified
					pc = wtypes[wt].eglow;
					cr = fire_r[y][x] + PIXR(pc);
					if (cr > 255) cr = 255;
					fire_r[y][x] = cr;
					cg = fire_g[y][x] + PIXG(pc);
					if (cg > 255) cg = 255;
					fire_g[y][x] = cg;
					cb = fire_b[y][x] + PIXB(pc);
					if (cb > 255) cb = 255;
					fire_b[y][x] = cb;

				}
			}
#endif
}

void Renderer::get_sign_pos(int i, int *x0, int *y0, int *w, int *h)
{
	std::vector<sign> signs = sim->signs;
	//Changing width if sign have special content
	if (signs[i].text == "{p}")
	{
		*w = Graphics::textwidth("Pressure: -000.00");
	}
	else if (signs[i].text == "{t}")
	{
		*w = Graphics::textwidth("Temp: 0000.00");
	}
	else if (sregexp(signs[i].text.c_str(), "^{c:[0-9]*|.*}$")==0)
	{
		int sldr, startm;
		char buff[256];
		memset(buff, 0, sizeof(buff));
		for (sldr=3; signs[i].text[sldr-1] != '|'; sldr++)
			startm = sldr + 1;

		sldr = startm;
		while (signs[i].text[sldr] != '}')
		{
			buff[sldr - startm] = signs[i].text[sldr];
			sldr++;
		}
		*w = Graphics::textwidth(buff) + 5;
	}
	else
	{
		*w = Graphics::textwidth(signs[i].text.c_str()) + 5;
	}
	*h = 14;
	*x0 = (signs[i].ju == 2) ? signs[i].x - *w :
	      (signs[i].ju == 1) ? signs[i].x - *w/2 : signs[i].x;
	*y0 = (signs[i].y > 18) ? signs[i].y - 18 : signs[i].y + 4;
}

void Renderer::DrawSigns()
{
	int i, j, x, y, w, h, dx, dy,mx,my,b=1,bq;
	std::vector<sign> signs = sim->signs;
#ifdef OGLR
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, partsFbo);
	glTranslated(0, MENUSIZE, 0);
#endif
	for (i=0; i < signs.size(); i++)
		if (signs[i].text.length())
		{
			char buff[256];  //Buffer
			get_sign_pos(i, &x, &y, &w, &h);
			g->clearrect(x, y, w, h);
			g->drawrect(x, y, w, h, 192, 192, 192, 255);

			//Displaying special information
			if (signs[i].text == "{p}")
			{
				float pressure = 0.0f;
				if (signs[i].x>=0 && signs[i].x<XRES && signs[i].y>=0 && signs[i].y<YRES)
					pressure = sim->pv[signs[i].y/CELL][signs[i].x/CELL];
				sprintf(buff, "Pressure: %3.2f", pressure);  //...pressure
				g->drawtext(x+3, y+3, buff, 255, 255, 255, 255);
			}
			else if (signs[i].text == "{t}")
			{
				if (signs[i].x>=0 && signs[i].x<XRES && signs[i].y>=0 && signs[i].y<YRES && sim->pmap[signs[i].y][signs[i].x])
					sprintf(buff, "Temp: %4.2f", sim->parts[sim->pmap[signs[i].y][signs[i].x]>>8].temp-273.15);  //...temperature
				else
					sprintf(buff, "Temp: 0.00");  //...temperature
				g->drawtext(x+3, y+3, buff, 255, 255, 255, 255);
			}
			else if (sregexp(signs[i].text.c_str(), "^{c:[0-9]*|.*}$")==0)
			{
				int sldr, startm;
				memset(buff, 0, sizeof(buff));
				for (sldr=3; signs[i].text[sldr-1] != '|'; sldr++)
					startm = sldr + 1;
				sldr = startm;
				while (signs[i].text[sldr] != '}')
				{
					buff[sldr - startm] = signs[i].text[sldr];
					sldr++;
				}
				g->drawtext(x+3, y+3, buff, 0, 191, 255, 255);
			}
			else 
			{
				g->drawtext(x+3, y+3, signs[i].text, 255, 255, 255, 255);
			}
				
			x = signs[i].x;
			y = signs[i].y;
			dx = 1 - signs[i].ju;
			dy = (signs[i].y > 18) ? -1 : 1;
#ifdef OGLR
			glBegin(GL_LINES);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glVertex2i(x, y);
			glVertex2i(x+(dx*4), y+(dy*4));
			glEnd();
#else
			for (j=0; j<4; j++)
			{
				g->blendpixel(x, y, 192, 192, 192, 255);
				x+=dx;
				y+=dy;
			}
#endif
			/*if (MSIGN==i)
			{
				bq = b;
				b = SDL_GetMouseState(&mx, &my);
				mx /= sdl_scale;
				my /= sdl_scale;
				signs[i].x = mx;
				signs[i].y = my;
			}*/
		}
#ifdef OGLR
	glTranslated(0, -MENUSIZE, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#endif
}

void Renderer::render_gravlensing()
{
#ifndef OGLR
	int nx, ny, rx, ry, gx, gy, bx, by, co;
	int r, g, b;
	pixel t;
	pixel *src = this->g->vid;
	pixel *dst = this->g->vid;
	for(nx = 0; nx < XRES; nx++)
	{
		for(ny = 0; ny < YRES; ny++)
		{
			co = (ny/CELL)*(XRES/CELL)+(nx/CELL);
			rx = (int)(nx-sim->gravx[co]*0.75f+0.5f);
			ry = (int)(ny-sim->gravy[co]*0.75f+0.5f);
			gx = (int)(nx-sim->gravx[co]*0.875f+0.5f);
			gy = (int)(ny-sim->gravy[co]*0.875f+0.5f);
			bx = (int)(nx-sim->gravx[co]+0.5f);
			by = (int)(ny-sim->gravy[co]+0.5f);
			if(rx > 0 && rx < XRES && ry > 0 && ry < YRES && gx > 0 && gx < XRES && gy > 0 && gy < YRES && bx > 0 && bx < XRES && by > 0 && by < YRES)
			{
				t = dst[ny*(XRES+BARSIZE)+nx];
				r = PIXR(src[ry*(XRES+BARSIZE)+rx]) + PIXR(t);
				g = PIXG(src[gy*(XRES+BARSIZE)+gx]) + PIXG(t);
				b = PIXB(src[by*(XRES+BARSIZE)+bx]) + PIXB(t);
				if (r>255)
					r = 255;
				if (g>255)
					g = 255;
				if (b>255)
					b = 255;
				dst[ny*(XRES+BARSIZE)+nx] = PIXRGB(r,g,b);
			}
		}
	}
#endif
}

void Renderer::render_fire()
{
#ifndef OGLR
	int i,j,x,y,r,g,b,nx,ny;
	for (j=0; j<YRES/CELL; j++)
		for (i=0; i<XRES/CELL; i++)
		{
			r = fire_r[j][i];
			g = fire_g[j][i];
			b = fire_b[j][i];
			if (r || g || b)
				for (y=-CELL; y<2*CELL; y++)
					for (x=-CELL; x<2*CELL; x++)
						this->g->addpixel(i*CELL+x, j*CELL+y, r, g, b, fire_alpha[y+CELL][x+CELL]);
			r *= 8;
			g *= 8;
			b *= 8;
			for (y=-1; y<2; y++)
				for (x=-1; x<2; x++)
					if ((x || y) && i+x>=0 && j+y>=0 && i+x<XRES/CELL && j+y<YRES/CELL)
					{
						r += fire_r[j+y][i+x];
						g += fire_g[j+y][i+x];
						b += fire_b[j+y][i+x];
					}
			r /= 16;
			g /= 16;
			b /= 16;
			fire_r[j][i] = r>4 ? r-4 : 0;
			fire_g[j][i] = g>4 ? g-4 : 0;
			fire_b[j][i] = b>4 ? b-4 : 0;
		}
#endif
}

float temp[CELL*3][CELL*3];
float fire_alphaf[CELL*3][CELL*3];
float glow_alphaf[11][11];
float blur_alphaf[7][7];
void Renderer::prepare_alpha(int size, float intensity)
{
	//TODO: implement size
	int x,y,i,j,c;
	float multiplier = 255.0f*intensity;

	memset(temp, 0, sizeof(temp));
	for (x=0; x<CELL; x++)
		for (y=0; y<CELL; y++)
			for (i=-CELL; i<CELL; i++)
				for (j=-CELL; j<CELL; j++)
					temp[y+CELL+j][x+CELL+i] += expf(-0.1f*(i*i+j*j));
	for (x=0; x<CELL*3; x++)
		for (y=0; y<CELL*3; y++)
			fire_alpha[y][x] = (int)(multiplier*temp[y][x]/(CELL*CELL));

#ifdef OGLR
	memset(fire_alphaf, 0, sizeof(fire_alphaf));
	for (x=0; x<CELL*3; x++)
		for (y=0; y<CELL*3; y++)
		{
			fire_alphaf[y][x] = intensity*temp[y][x]/((float)(CELL*CELL));
		}
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, fireAlpha);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, CELL*3, CELL*3, GL_ALPHA, GL_FLOAT, fire_alphaf);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	memset(glow_alphaf, 0, sizeof(glow_alphaf));

	c = 5;

	glow_alphaf[c][c-1] = 0.4f;
	glow_alphaf[c][c+1] = 0.4f;
	glow_alphaf[c-1][c] = 0.4f;
	glow_alphaf[c+1][c] = 0.4f;
	for (x = 1; x < 6; x++) {
		glow_alphaf[c][c-x] += 0.02f;
		glow_alphaf[c][c+x] += 0.02f;
		glow_alphaf[c-x][c] += 0.02f;
		glow_alphaf[c+x][c] += 0.02f;
		for (y = 1; y < 6; y++) {
			if(x + y > 7)
				continue;
			glow_alphaf[c+x][c-y] += 0.02f;
			glow_alphaf[c-x][c+y] += 0.02f;
			glow_alphaf[c+x][c+y] += 0.02f;
			glow_alphaf[c-x][c-y] += 0.02f;
		}
	}

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, glowAlpha);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 11, 11, GL_ALPHA, GL_FLOAT, glow_alphaf);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	c = 3;

	for (x=-3; x<4; x++)
	{
		for (y=-3; y<4; y++)
		{
			if (abs(x)+abs(y) <2 && !(abs(x)==2||abs(y)==2))
				blur_alphaf[c+x][c-y] = 0.11f;
			if (abs(x)+abs(y) <=3 && abs(x)+abs(y))
				blur_alphaf[c+x][c-y] = 0.08f;
			if (abs(x)+abs(y) == 2)
				blur_alphaf[c+x][c-y] = 0.04f;
		}
	}

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, blurAlpha);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 7, 7, GL_ALPHA, GL_FLOAT, blur_alphaf);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
#endif
}

void Renderer::render_parts()
{
	int deca, decr, decg, decb, cola, colr, colg, colb, firea, firer, fireg, fireb, pixel_mode, q, i, t, nx, ny, x, y, caddress;
	int orbd[4] = {0, 0, 0, 0}, orbl[4] = {0, 0, 0, 0};
	float gradv, flicker, fnx, fny;
	Particle * parts;
	part_transition *ptransitions;
	Element *elements;
	if(!sim)
		return;
	parts = sim->parts;
	elements = sim->elements;
#ifdef OGLR
	int cfireV = 0, cfireC = 0, cfire = 0;
	int csmokeV = 0, csmokeC = 0, csmoke = 0;
	int cblobV = 0, cblobC = 0, cblob = 0;
	int cblurV = 0, cblurC = 0, cblur = 0;
	int cglowV = 0, cglowC = 0, cglow = 0;
	int cflatV = 0, cflatC = 0, cflat = 0;
	int caddV = 0, caddC = 0, cadd = 0;
	int clineV = 0, clineC = 0, cline = 0;
	GLint origBlendSrc, origBlendDst;

	glGetIntegerv(GL_BLEND_SRC, &origBlendSrc);
	glGetIntegerv(GL_BLEND_DST, &origBlendDst);
	//Render to the particle FBO
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, partsFbo);
	glTranslated(0, MENUSIZE, 0);
#else
	/*if (GRID_MODE)//draws the grid
	{
		for (ny=0; ny<YRES; ny++)
			for (nx=0; nx<XRES; nx++)
			{
				if (ny%(4*GRID_MODE)==0)
					blendpixel(nx, ny, 100, 100, 100, 80);
				if (nx%(4*GRID_MODE)==0)
					blendpixel(nx, ny, 100, 100, 100, 80);
			}
	}*/
#endif
	for(i = 0; i<=sim->parts_lastActiveIndex; i++) {
		if (sim->parts[i].type) {
			t = sim->parts[i].type;

			nx = (int)(sim->parts[i].x+0.5f);
			ny = (int)(sim->parts[i].y+0.5f);
			fnx = sim->parts[i].x;
			fny = sim->parts[i].y;

			if((sim->photons[ny][nx]&0xFF) && !(elements[t].Properties & TYPE_ENERGY))
				continue;

			//Defaults
			pixel_mode = 0 | PMODE_FLAT;
			cola = 255;
			colr = PIXR(elements[t].Colour);
			colg = PIXG(elements[t].Colour);
			colb = PIXB(elements[t].Colour);
			firea = 0;

			deca = (sim->parts[i].dcolour>>24)&0xFF;
			decr = (sim->parts[i].dcolour>>16)&0xFF;
			decg = (sim->parts[i].dcolour>>8)&0xFF;
			decb = (sim->parts[i].dcolour)&0xFF;

			{
				if (graphicscache[t].isready)
				{
					pixel_mode = graphicscache[t].pixel_mode;
					cola = graphicscache[t].cola;
					colr = graphicscache[t].colr;
					colg = graphicscache[t].colg;
					colb = graphicscache[t].colb;
					firea = graphicscache[t].firea;
					firer = graphicscache[t].firer;
					fireg = graphicscache[t].fireg;
					fireb = graphicscache[t].fireb;
				}
				else if(!(colour_mode & COLOUR_BASC))
				{
					if (elements[t].Graphics)
					{
						if ((*(elements[t].Graphics))(this, &(sim->parts[i]), nx, ny, &pixel_mode, &cola, &colr, &colg, &colb, &firea, &firer, &fireg, &fireb)) //That's a lot of args, a struct might be better
						{
							graphicscache[t].isready = 1;
							graphicscache[t].pixel_mode = pixel_mode;
							graphicscache[t].cola = cola;
							graphicscache[t].colr = colr;
							graphicscache[t].colg = colg;
							graphicscache[t].colb = colb;
							graphicscache[t].firea = firea;
							graphicscache[t].firer = firer;
							graphicscache[t].fireg = fireg;
							graphicscache[t].fireb = fireb;
						}
					}
					else
					{
						graphicscache[t].isready = 1;
						graphicscache[t].pixel_mode = pixel_mode;
						graphicscache[t].cola = cola;
						graphicscache[t].colr = colr;
						graphicscache[t].colg = colg;
						graphicscache[t].colb = colb;
						graphicscache[t].firea = firea;
						graphicscache[t].firer = firer;
						graphicscache[t].fireg = fireg;
						graphicscache[t].fireb = fireb;
					}
				}
				if((elements[t].Properties & PROP_HOT_GLOW) && sim->parts[i].temp>(elements[t].HighTemperature-800.0f))
				{
					gradv = 3.1415/(2*elements[t].HighTemperature-(elements[t].HighTemperature-800.0f));
					caddress = (sim->parts[i].temp>elements[t].HighTemperature)?elements[t].HighTemperature-(elements[t].HighTemperature-800.0f):sim->parts[i].temp-(elements[t].HighTemperature-800.0f);
					colr += sin(gradv*caddress) * 226;;
					colg += sin(gradv*caddress*4.55 +3.14) * 34;
					colb += sin(gradv*caddress*2.22 +3.14) * 64;
				}

				if((pixel_mode & FIRE_ADD) && !(render_mode & FIRE_ADD))
					pixel_mode |= PMODE_GLOW;
				if((pixel_mode & FIRE_BLEND) && !(render_mode & FIRE_BLEND))
					pixel_mode |= PMODE_BLUR;
				if((pixel_mode & PMODE_BLUR) && !(render_mode & PMODE_BLUR))
					pixel_mode |= PMODE_FLAT;
				if((pixel_mode & PMODE_GLOW) && !(render_mode & PMODE_GLOW))
					pixel_mode |= PMODE_BLEND;
				if (render_mode & PMODE_BLOB)
					pixel_mode |= PMODE_BLOB;

				pixel_mode &= render_mode;

				//Alter colour based on display mode
				if(colour_mode & COLOUR_HEAT)
				{
					caddress = restrict_flt((int)( restrict_flt((float)(sim->parts[i].temp+(-MIN_TEMP)), 0.0f, MAX_TEMP+(-MIN_TEMP)) / ((MAX_TEMP+(-MIN_TEMP))/1024) ) *3, 0.0f, (1024.0f*3)-3);
					firea = 255;
					firer = colr = (unsigned char)color_data[caddress];
					fireg = colg = (unsigned char)color_data[caddress+1];
					fireb = colb = (unsigned char)color_data[caddress+2];
					cola = 255;
					if(pixel_mode & (FIREMODE | PMODE_GLOW)) pixel_mode = (pixel_mode & ~(FIREMODE|PMODE_GLOW)) | PMODE_BLUR;
				}
				else if(colour_mode & COLOUR_LIFE)
				{
					gradv = 0.4f;
					if (!(sim->parts[i].life<5))
						q = sqrt(sim->parts[i].life);
					else
						q = sim->parts[i].life;
					colr = colg = colb = sin(gradv*q) * 100 + 128;
					cola = 255;
					if(pixel_mode & (FIREMODE | PMODE_GLOW)) pixel_mode = (pixel_mode & ~(FIREMODE|PMODE_GLOW)) | PMODE_BLUR;
				}
				else if(colour_mode & COLOUR_BASC)
				{
					colr = PIXR(elements[t].Colour);
					colg = PIXG(elements[t].Colour);
					colb = PIXB(elements[t].Colour);
					pixel_mode = PMODE_FLAT;
				}

				//Apply decoration colour
				if(!(colour_mode & ~COLOUR_GRAD))
				{
					if(!(pixel_mode & NO_DECO) && decorations_enable)
					{
						colr = (deca*decr + (255-deca)*colr) >> 8;
						colg = (deca*decg + (255-deca)*colg) >> 8;
						colb = (deca*decb + (255-deca)*colb) >> 8;
					}

					if((pixel_mode & DECO_FIRE) && decorations_enable)
					{
						firer = (deca*decr + (255-deca)*firer) >> 8;
						fireg = (deca*decg + (255-deca)*fireg) >> 8;
						fireb = (deca*decb + (255-deca)*fireb) >> 8;
					}
				}

				if (colour_mode & COLOUR_GRAD)
				{
					float frequency = 0.05;
					int q = sim->parts[i].temp-40;
					colr = sin(frequency*q) * 16 + colr;
					colg = sin(frequency*q) * 16 + colg;
					colb = sin(frequency*q) * 16 + colb;
					if(pixel_mode & (FIREMODE | PMODE_GLOW)) pixel_mode = (pixel_mode & ~(FIREMODE|PMODE_GLOW)) | PMODE_BLUR;
				}

	#ifndef OGLR
				//All colours are now set, check ranges
				if(colr>255) colr = 255;
				else if(colr<0) colr = 0;
				if(colg>255) colg = 255;
				else if(colg<0) colg = 0;
				if(colb>255) colb = 255;
				else if(colb<0) colb = 0;
				if(cola>255) cola = 255;
				else if(cola<0) cola = 0;
	#endif

				//Pixel rendering
				if(pixel_mode & PSPEC_STICKMAN)
				{
					char buff[20];  //Buffer for HP
					int s;
					int legr, legg, legb;
					playerst *cplayer;
					if(t==PT_STKM)
						cplayer = &sim->player;
					else if(t==PT_STKM2)
						cplayer = &sim->player2;
					else if(t==PT_FIGH)
						cplayer = &sim->fighters[(unsigned char)sim->parts[i].tmp];
					else
						continue;

/*					if (mousex>(nx-3) && mousex<(nx+3) && mousey<(ny+3) && mousey>(ny-3)) //If mous is in the head
					{
						sprintf(buff, "%3d", sim->parts[i].life);  //Show HP
						g->drawtext(mousex-8-2*(sim->parts[i].life<100)-2*(sim->parts[i].life<10), mousey-12, buff, 255, 255, 255, 255);
					}*/

					if (colour_mode!=COLOUR_HEAT)
					{
						if (cplayer->elem<PT_NUM)
						{
							colr = PIXR(elements[cplayer->elem].Colour);
							colg = PIXG(elements[cplayer->elem].Colour);
							colb = PIXB(elements[cplayer->elem].Colour);
						}
						else
						{
							colr = 0x80;
							colg = 0x80;
							colb = 0xFF;
						}
					}
#ifdef OGLR
					glColor4f(((float)colr)/255.0f, ((float)colg)/255.0f, ((float)colb)/255.0f, 1.0f);
					glBegin(GL_LINE_STRIP);
					if(t==PT_FIGH)
					{
						glVertex2f(fnx, fny+2);
						glVertex2f(fnx+2, fny);
						glVertex2f(fnx, fny-2);
						glVertex2f(fnx-2, fny);
						glVertex2f(fnx, fny+2);
					}
					else
					{
						glVertex2f(fnx-2, fny-2);
						glVertex2f(fnx+2, fny-2);
						glVertex2f(fnx+2, fny+2);
						glVertex2f(fnx-2, fny+2);
						glVertex2f(fnx-2, fny-2);
					}
					glEnd();
					glBegin(GL_LINES);

					if (colour_mode!=COLOUR_HEAT)
					{
						if (t==PT_STKM2)
							glColor4f(100.0f/255.0f, 100.0f/255.0f, 1.0f, 1.0f);
						else
							glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
					}

					glVertex2f(nx, ny+3);
					glVertex2f(cplayer->legs[0], cplayer->legs[1]);

					glVertex2f(cplayer->legs[0], cplayer->legs[1]);
					glVertex2f(cplayer->legs[4], cplayer->legs[5]);

					glVertex2f(nx, ny+3);
					glVertex2f(cplayer->legs[8], cplayer->legs[9]);

					glVertex2f(cplayer->legs[8], cplayer->legs[9]);
					glVertex2f(cplayer->legs[12], cplayer->legs[13]);
					glEnd();
#else
					s = XRES+BARSIZE;

					if (t==PT_STKM2)
					{
						legr = 100;
						legg = 100;
						legb = 255;
					}
					else
					{
						legr = 255;
						legg = 255;
						legb = 255;
					}

					if (colour_mode==COLOUR_HEAT)
					{
						legr = colr;
						legg = colg;
						legb = colb;
					}

					//head
					if(t==PT_FIGH)
					{
						g->draw_line(nx, ny+2, nx+2, ny, colr, colg, colb, s);
						g->draw_line(nx+2, ny, nx, ny-2, colr, colg, colb, s);
						g->draw_line(nx, ny-2, nx-2, ny, colr, colg, colb, s);
						g->draw_line(nx-2, ny, nx, ny+2, colr, colg, colb, s);
					}
					else
					{
						g->draw_line(nx-2, ny+2, nx+2, ny+2, colr, colg, colb, s);
						g->draw_line(nx-2, ny-2, nx+2, ny-2, colr, colg, colb, s);
						g->draw_line(nx-2, ny-2, nx-2, ny+2, colr, colg, colb, s);
						g->draw_line(nx+2, ny-2, nx+2, ny+2, colr, colg, colb, s);
					}
					//legs
					g->draw_line(nx, ny+3, cplayer->legs[0], cplayer->legs[1], legr, legg, legb, s);
					g->draw_line(cplayer->legs[0], cplayer->legs[1], cplayer->legs[4], cplayer->legs[5], legr, legg, legb, s);
					g->draw_line(nx, ny+3, cplayer->legs[8], cplayer->legs[9], legr, legg, legb, s);
					g->draw_line(cplayer->legs[8], cplayer->legs[9], cplayer->legs[12], cplayer->legs[13], legr, legg, legb, s);
#endif
				}
				if(pixel_mode & PMODE_FLAT)
				{
#ifdef OGLR
                    flatV[cflatV++] = nx;
                    flatV[cflatV++] = ny;
                    flatC[cflatC++] = ((float)colr)/255.0f;
                    flatC[cflatC++] = ((float)colg)/255.0f;
                    flatC[cflatC++] = ((float)colb)/255.0f;
                    flatC[cflatC++] = 1.0f;
                    cflat++;
#else
                    g->vid[ny*(XRES+BARSIZE)+nx] = PIXRGB(colr,colg,colb);
#endif
				}
				if(pixel_mode & PMODE_BLEND)
				{
#ifdef OGLR
                    flatV[cflatV++] = nx;
                    flatV[cflatV++] = ny;
                    flatC[cflatC++] = ((float)colr)/255.0f;
                    flatC[cflatC++] = ((float)colg)/255.0f;
                    flatC[cflatC++] = ((float)colb)/255.0f;
                    flatC[cflatC++] = ((float)cola)/255.0f;
                    cflat++;
#else
                    g->blendpixel(nx, ny, colr, colg, colb, cola);
#endif
				}
				if(pixel_mode & PMODE_ADD)
				{
#ifdef OGLR
                    addV[caddV++] = nx;
                    addV[caddV++] = ny;
                    addC[caddC++] = ((float)colr)/255.0f;
                    addC[caddC++] = ((float)colg)/255.0f;
                    addC[caddC++] = ((float)colb)/255.0f;
                    addC[caddC++] = ((float)cola)/255.0f;
                    cadd++;
#else
                    g->addpixel(nx, ny, colr, colg, colb, cola);
#endif
				}
				if(pixel_mode & PMODE_BLOB)
				{
#ifdef OGLR
                    blobV[cblobV++] = nx;
                    blobV[cblobV++] = ny;
                    blobC[cblobC++] = ((float)colr)/255.0f;
                    blobC[cblobC++] = ((float)colg)/255.0f;
                    blobC[cblobC++] = ((float)colb)/255.0f;
                    blobC[cblobC++] = 1.0f;
                    cblob++;
#else
                    g->vid[ny*(XRES+BARSIZE)+nx] = PIXRGB(colr,colg,colb);

					g->blendpixel(nx+1, ny, colr, colg, colb, 223);
					g->blendpixel(nx-1, ny, colr, colg, colb, 223);
					g->blendpixel(nx, ny+1, colr, colg, colb, 223);
					g->blendpixel(nx, ny-1, colr, colg, colb, 223);

					g->blendpixel(nx+1, ny-1, colr, colg, colb, 112);
					g->blendpixel(nx-1, ny-1, colr, colg, colb, 112);
					g->blendpixel(nx+1, ny+1, colr, colg, colb, 112);
					g->blendpixel(nx-1, ny+1, colr, colg, colb, 112);
#endif
				}
				if(pixel_mode & PMODE_GLOW)
				{
					int cola1 = (5*cola)/255;
#ifdef OGLR
                    glowV[cglowV++] = nx;
                    glowV[cglowV++] = ny;
                    glowC[cglowC++] = ((float)colr)/255.0f;
                    glowC[cglowC++] = ((float)colg)/255.0f;
                    glowC[cglowC++] = ((float)colb)/255.0f;
                    glowC[cglowC++] = 1.0f;
                    cglow++;
#else
                    g->addpixel(nx, ny, colr, colg, colb, (192*cola)/255);
                    g->addpixel(nx+1, ny, colr, colg, colb, (96*cola)/255);
                    g->addpixel(nx-1, ny, colr, colg, colb, (96*cola)/255);
                    g->addpixel(nx, ny+1, colr, colg, colb, (96*cola)/255);
                    g->addpixel(nx, ny-1, colr, colg, colb, (96*cola)/255);

					for (x = 1; x < 6; x++) {
						g->addpixel(nx, ny-x, colr, colg, colb, cola1);
						g->addpixel(nx, ny+x, colr, colg, colb, cola1);
						g->addpixel(nx-x, ny, colr, colg, colb, cola1);
						g->addpixel(nx+x, ny, colr, colg, colb, cola1);
						for (y = 1; y < 6; y++) {
							if(x + y > 7)
								continue;
							g->addpixel(nx+x, ny-y, colr, colg, colb, cola1);
							g->addpixel(nx-x, ny+y, colr, colg, colb, cola1);
							g->addpixel(nx+x, ny+y, colr, colg, colb, cola1);
							g->addpixel(nx-x, ny-y, colr, colg, colb, cola1);
						}
					}
#endif
				}
				if(pixel_mode & PMODE_BLUR)
				{
#ifdef OGLR
                    blurV[cblurV++] = nx;
                    blurV[cblurV++] = ny;
                    blurC[cblurC++] = ((float)colr)/255.0f;
                    blurC[cblurC++] = ((float)colg)/255.0f;
                    blurC[cblurC++] = ((float)colb)/255.0f;
                    blurC[cblurC++] = 1.0f;
                    cblur++;
#else
					for (x=-3; x<4; x++)
					{
						for (y=-3; y<4; y++)
						{
							if (abs(x)+abs(y) <2 && !(abs(x)==2||abs(y)==2))
								g->blendpixel(x+nx, y+ny, colr, colg, colb, 30);
							if (abs(x)+abs(y) <=3 && abs(x)+abs(y))
								g->blendpixel(x+nx, y+ny, colr, colg, colb, 20);
							if (abs(x)+abs(y) == 2)
								g->blendpixel(x+nx, y+ny, colr, colg, colb, 10);
						}
					}
#endif
				}
				if(pixel_mode & PMODE_SPARK)
				{
					flicker = rand()%20;
#ifdef OGLR
					//Oh god, this is awful
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx-5;
				    lineV[clineV++] = fny;
				    cline++;

				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 1.0f - ((float)flicker)/30;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny;
				    cline++;

				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx+5;
				    lineV[clineV++] = fny;
				    cline++;

				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny-5;
				    cline++;

				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 1.0f - ((float)flicker)/30;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny;
				    cline++;

				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny+5;
				    cline++;
#else
					gradv = 4*sim->parts[i].life + flicker;
					for (x = 0; gradv>0.5; x++) {
						g->addpixel(nx+x, ny, colr, colg, colb, gradv);
						g->addpixel(nx-x, ny, colr, colg, colb, gradv);

						g->addpixel(nx, ny+x, colr, colg, colb, gradv);
						g->addpixel(nx, ny-x, colr, colg, colb, gradv);
						gradv = gradv/1.5f;
					}
#endif
				}
				if(pixel_mode & PMODE_FLARE)
				{
					flicker = rand()%20;
#ifdef OGLR
					//Oh god, this is awful
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx-10;
				    lineV[clineV++] = fny;
				    cline++;

				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 1.0f - ((float)flicker)/40;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny;
				    cline++;

				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx+10;
				    lineV[clineV++] = fny;
				    cline++;

				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny-10;
				    cline++;

				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 1.0f - ((float)flicker)/30;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny;
				    cline++;

				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny+10;
				    cline++;
#else
					gradv = flicker + fabs(parts[i].vx)*17 + fabs(sim->parts[i].vy)*17;
					g->blendpixel(nx, ny, colr, colg, colb, (gradv*4)>255?255:(gradv*4) );
					g->blendpixel(nx+1, ny, colr, colg, colb, (gradv*2)>255?255:(gradv*2) );
					g->blendpixel(nx-1, ny, colr, colg, colb, (gradv*2)>255?255:(gradv*2) );
					g->blendpixel(nx, ny+1, colr, colg, colb, (gradv*2)>255?255:(gradv*2) );
					g->blendpixel(nx, ny-1, colr, colg, colb, (gradv*2)>255?255:(gradv*2) );
					if (gradv>255) gradv=255;
					g->blendpixel(nx+1, ny-1, colr, colg, colb, gradv);
					g->blendpixel(nx-1, ny-1, colr, colg, colb, gradv);
					g->blendpixel(nx+1, ny+1, colr, colg, colb, gradv);
					g->blendpixel(nx-1, ny+1, colr, colg, colb, gradv);
					for (x = 1; gradv>0.5; x++) {
						g->addpixel(nx+x, ny, colr, colg, colb, gradv);
						g->addpixel(nx-x, ny, colr, colg, colb, gradv);
						g->addpixel(nx, ny+x, colr, colg, colb, gradv);
						g->addpixel(nx, ny-x, colr, colg, colb, gradv);
						gradv = gradv/1.2f;
					}
#endif
				}
				if(pixel_mode & PMODE_LFLARE)
				{
					flicker = rand()%20;
#ifdef OGLR
					//Oh god, this is awful
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx-70;
				    lineV[clineV++] = fny;
				    cline++;

				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 1.0f - ((float)flicker)/30;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny;
				    cline++;

				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx+70;
				    lineV[clineV++] = fny;
				    cline++;

				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny-70;
				    cline++;

				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 1.0f - ((float)flicker)/50;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny;
				    cline++;

				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny+70;
				    cline++;
#else
					gradv = flicker + fabs(parts[i].vx)*17 + fabs(parts[i].vy)*17;
					g->blendpixel(nx, ny, colr, colg, colb, (gradv*4)>255?255:(gradv*4) );
					g->blendpixel(nx+1, ny, colr, colg, colb, (gradv*2)>255?255:(gradv*2) );
					g->blendpixel(nx-1, ny, colr, colg, colb, (gradv*2)>255?255:(gradv*2) );
					g->blendpixel(nx, ny+1, colr, colg, colb, (gradv*2)>255?255:(gradv*2) );
					g->blendpixel(nx, ny-1, colr, colg, colb, (gradv*2)>255?255:(gradv*2) );
					if (gradv>255) gradv=255;
					g->blendpixel(nx+1, ny-1, colr, colg, colb, gradv);
					g->blendpixel(nx-1, ny-1, colr, colg, colb, gradv);
					g->blendpixel(nx+1, ny+1, colr, colg, colb, gradv);
					g->blendpixel(nx-1, ny+1, colr, colg, colb, gradv);
					for (x = 1; gradv>0.5; x++) {
						g->addpixel(nx+x, ny, colr, colg, colb, gradv);
						g->addpixel(nx-x, ny, colr, colg, colb, gradv);
						g->addpixel(nx, ny+x, colr, colg, colb, gradv);
						g->addpixel(nx, ny-x, colr, colg, colb, gradv);
						gradv = gradv/1.01f;
					}
#endif
				}
				if (pixel_mode & EFFECT_GRAVIN)
				{
					int nxo = 0;
					int nyo = 0;
					int r;
					int fire_rv = 0;
					float drad = 0.0f;
					float ddist = 0.0f;
					sim->orbitalparts_get(parts[i].life, parts[i].ctype, orbd, orbl);
					for (r = 0; r < 4; r++) {
						ddist = ((float)orbd[r])/16.0f;
						drad = (M_PI * ((float)orbl[r]) / 180.0f)*1.41f;
						nxo = ddist*cos(drad);
						nyo = ddist*sin(drad);
						if (ny+nyo>0 && ny+nyo<YRES && nx+nxo>0 && nx+nxo<XRES)
							g->addpixel(nx+nxo, ny+nyo, colr, colg, colb, 255-orbd[r]);
					}
				}
				if (pixel_mode & EFFECT_GRAVOUT)
				{
					int nxo = 0;
					int nyo = 0;
					int r;
					int fire_bv = 0;
					float drad = 0.0f;
					float ddist = 0.0f;
					sim->orbitalparts_get(parts[i].life, parts[i].ctype, orbd, orbl);
					for (r = 0; r < 4; r++) {
						ddist = ((float)orbd[r])/16.0f;
						drad = (M_PI * ((float)orbl[r]) / 180.0f)*1.41f;
						nxo = ddist*cos(drad);
						nyo = ddist*sin(drad);
						if (ny+nyo>0 && ny+nyo<YRES && nx+nxo>0 && nx+nxo<XRES)
							g->addpixel(nx+nxo, ny+nyo, colr, colg, colb, 255-orbd[r]);
					}
				}
				//Fire effects
				if(firea && (pixel_mode & FIRE_BLEND))
				{
#ifdef OGLR
                    smokeV[csmokeV++] = nx;
                    smokeV[csmokeV++] = ny;
                    smokeC[csmokeC++] = ((float)firer)/255.0f;
                    smokeC[csmokeC++] = ((float)fireg)/255.0f;
                    smokeC[csmokeC++] = ((float)fireb)/255.0f;
                    smokeC[csmokeC++] = ((float)firea)/255.0f;
                    csmoke++;
#else
					firea /= 2;
					fire_r[ny/CELL][nx/CELL] = (firea*firer + (255-firea)*fire_r[ny/CELL][nx/CELL]) >> 8;
					fire_g[ny/CELL][nx/CELL] = (firea*fireg + (255-firea)*fire_g[ny/CELL][nx/CELL]) >> 8;
					fire_b[ny/CELL][nx/CELL] = (firea*fireb + (255-firea)*fire_b[ny/CELL][nx/CELL]) >> 8;
#endif
				}
				if(firea && (pixel_mode & FIRE_ADD))
				{
#ifdef OGLR
                    fireV[cfireV++] = nx;
                    fireV[cfireV++] = ny;
                    fireC[cfireC++] = ((float)firer)/255.0f;
                    fireC[cfireC++] = ((float)fireg)/255.0f;
                    fireC[cfireC++] = ((float)fireb)/255.0f;
                    fireC[cfireC++] = ((float)firea)/255.0f;
                    cfire++;
#else
					firea /= 8;
					firer = ((firea*firer) >> 8) + fire_r[ny/CELL][nx/CELL];
					fireg = ((firea*fireg) >> 8) + fire_g[ny/CELL][nx/CELL];
					fireb = ((firea*fireb) >> 8) + fire_b[ny/CELL][nx/CELL];

					if(firer>255)
						firer = 255;
					if(fireg>255)
						fireg = 255;
					if(fireb>255)
						fireb = 255;

					fire_r[ny/CELL][nx/CELL] = firer;
					fire_g[ny/CELL][nx/CELL] = fireg;
					fire_b[ny/CELL][nx/CELL] = fireb;
#endif
				}
			}
		}
	}
#ifdef OGLR

        //Go into array mode
        glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_VERTEX_ARRAY);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

 		if(cflat)
		{
			// -- BEGIN FLAT -- //
			//Set point size (size of fire texture)
			glPointSize(1.0f);

		    glColorPointer(4, GL_FLOAT, 0, &flatC[0]);
		    glVertexPointer(2, GL_INT, 0, &flatV[0]);

		    glDrawArrays(GL_POINTS, 0, cflat);

		    //Clear some stuff we set
		    // -- END FLAT -- //
        }

        if(cblob)
		{
		    // -- BEGIN BLOB -- //
			glEnable( GL_POINT_SMOOTH ); //Blobs!
			glPointSize(2.5f);

		    glColorPointer(4, GL_FLOAT, 0, &blobC[0]);
		    glVertexPointer(2, GL_INT, 0, &blobV[0]);

		    glDrawArrays(GL_POINTS, 0, cblob);

		    //Clear some stuff we set
		    glDisable( GL_POINT_SMOOTH );
		    // -- END BLOB -- //
		}

        if(cglow || cblur)
        {
        	// -- BEGIN GLOW -- //
			//Start and prepare fire program
			glEnable(GL_TEXTURE_2D);
			glUseProgram(fireProg);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, glowAlpha);
			glUniform1i(glGetUniformLocation(fireProg, "fireAlpha"), 0);

			glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);

			//Make sure we can use texture coords on points
			glEnable(GL_POINT_SPRITE);
			glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
			glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);

			//Set point size (size of fire texture)
			glPointSize(11.0f);

			glBlendFunc(GL_SRC_ALPHA, GL_ONE);

			if(cglow)
			{
				glColorPointer(4, GL_FLOAT, 0, &glowC[0]);
				glVertexPointer(2, GL_INT, 0, &glowV[0]);

				glDrawArrays(GL_POINTS, 0, cglow);
			}

			glPointSize(7.0f);

			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			if(cblur)
			{
				glBindTexture(GL_TEXTURE_2D, blurAlpha);

				glColorPointer(4, GL_FLOAT, 0, &blurC[0]);
				glVertexPointer(2, GL_INT, 0, &blurV[0]);

				glDrawArrays(GL_POINTS, 0, cblur);
			}

		    //Clear some stuff we set
		    glDisable(GL_POINT_SPRITE);
			glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
			glUseProgram(0);
			glBindTexture(GL_TEXTURE_2D, 0);
			glDisable(GL_TEXTURE_2D);
		    // -- END GLOW -- //
        }

 		if(cadd)
		{
			// -- BEGIN ADD -- //
			//Set point size (size of fire texture)
			glPointSize(1.0f);

		    glColorPointer(4, GL_FLOAT, 0, &addC[0]);
		    glVertexPointer(2, GL_INT, 0, &addV[0]);

			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		    glDrawArrays(GL_POINTS, 0, cadd);
		    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		    //Clear some stuff we set
		    // -- END ADD -- //
        }

        if(cline)
		{
			// -- BEGIN LINES -- //
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glEnable( GL_LINE_SMOOTH );
		    glColorPointer(4, GL_FLOAT, 0, &lineC[0]);
		    glVertexPointer(2, GL_FLOAT, 0, &lineV[0]);

		    glDrawArrays(GL_LINE_STRIP, 0, cline);

		    //Clear some stuff we set
		    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		    glDisable(GL_LINE_SMOOTH);
		    // -- END LINES -- //
        }

		if(cfire || csmoke)
		{
			// -- BEGIN FIRE -- //
			//Start and prepare fire program
			glEnable(GL_TEXTURE_2D);
			glUseProgram(fireProg);
			//glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fireAlpha);
			glUniform1i(glGetUniformLocation(fireProg, "fireAlpha"), 0);

			//Make sure we can use texture coords on points
			glEnable(GL_POINT_SPRITE);
			glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
			glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);

			//Set point size (size of fire texture)
			glPointSize(CELL*3);

			glBlendFunc(GL_SRC_ALPHA, GL_ONE);

			if(cfire)
			{
				glColorPointer(4, GL_FLOAT, 0, &fireC[0]);
				glVertexPointer(2, GL_INT, 0, &fireV[0]);

				glDrawArrays(GL_POINTS, 0, cfire);
			}

			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			if(csmoke)
			{
				glColorPointer(4, GL_FLOAT, 0, &smokeC[0]);
				glVertexPointer(2, GL_INT, 0, &smokeV[0]);

				glDrawArrays(GL_POINTS, 0, csmoke);
			}

		    //Clear some stuff we set
		    glDisable(GL_POINT_SPRITE);
			glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
			glUseProgram(0);
			glBindTexture(GL_TEXTURE_2D, 0);
			glDisable(GL_TEXTURE_2D);
		    // -- END FIRE -- //
        }

        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);

        //Reset FBO
        glTranslated(0, -MENUSIZE, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        glBlendFunc(origBlendSrc, origBlendDst);
#endif
}

void Renderer::draw_other() // EMP effect
{
	int i, j;
	//if (emp_decor>0 && !sys_pause) emp_decor-=emp_decor/25+2; TODO: Render should render only, do not change simulation state
	if (emp_decor>40) emp_decor=40;
	if (emp_decor<0) emp_decor = 0;
	if (!(display_mode & DISPLAY_EFFE)) // no in nothing mode
		return;
	if (emp_decor>0)
	{
#ifdef OGLR
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, partsFbo);
		glTranslated(0, MENUSIZE, 0);
		float femp_decor = ((float)emp_decor)/255.0f;
		/*int r=emp_decor*2.5, g=100+emp_decor*1.5, b=255;
		int a=(1.0*emp_decor/110)*255;
		if (r>255) r=255;
		if (g>255) g=255;
		if (b>255) g=255;
		if (a>255) a=255;*/
		glBegin(GL_QUADS);
		glColor4f(femp_decor*2.5f, 0.4f+femp_decor*1.5f, 1.0f+femp_decor*1.5f, femp_decor/0.44f);
		glVertex2f(0, MENUSIZE);
		glVertex2f(XRES, MENUSIZE);
		glVertex2f(XRES, YRES+MENUSIZE);
		glVertex2f(0, YRES+MENUSIZE);
		glEnd();
		glTranslated(0, -MENUSIZE, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#else
		int r=emp_decor*2.5, g=100+emp_decor*1.5, b=255;
		int a=(1.0*emp_decor/110)*255;
		if (r>255) r=255;
		if (g>255) g=255;
		if (b>255) g=255;
		if (a>255) a=255;
		for (j=0; j<YRES; j++)
			for (i=0; i<XRES; i++)
			{
				this->g->blendpixel(i, j, r, g, b, a);
			}
#endif
	}
}

void Renderer::draw_grav()
{
	int x, y, i, ca;
	float nx, ny, dist;

	for (y=0; y<YRES/CELL; y++)
	{
		for (x=0; x<XRES/CELL; x++)
		{
			ca = y*(XRES/CELL)+x;
			if(fabsf(sim->gravx[ca]) <= 0.001f && fabsf(sim->gravy[ca]) <= 0.001f)
				continue;
			nx = x*CELL;
			ny = y*CELL;
			dist = fabsf(sim->gravy[ca])+fabsf(sim->gravx[ca]);
			for(i = 0; i < 4; i++)
			{
				nx -= sim->gravx[ca]*0.5f;
				ny -= sim->gravy[ca]*0.5f;
				g->addpixel((int)(nx+0.5f), (int)(ny+0.5f), 255, 255, 255, (int)(dist*20.0f));
			}
		}
	}
}

void Renderer::draw_air()
{
#ifndef OGLR
	if(!(display_mode & DISPLAY_AIR))
		return;
	int x, y, i, j;
	float (*pv)[XRES/CELL] = sim->air->pv;
	float (*hv)[XRES/CELL] = sim->air->hv;
	float (*vx)[XRES/CELL] = sim->air->vx;
	float (*vy)[XRES/CELL] = sim->air->vy;
	pixel c;
	for (y=0; y<YRES/CELL; y++)
		for (x=0; x<XRES/CELL; x++)
		{
			if (display_mode & DISPLAY_AIRP)
			{
				if (pv[y][x] > 0.0f)
					c  = PIXRGB(clamp_flt(pv[y][x], 0.0f, 8.0f), 0, 0);//positive pressure is red!
				else
					c  = PIXRGB(0, 0, clamp_flt(-pv[y][x], 0.0f, 8.0f));//negative pressure is blue!
			}
			else if (display_mode & DISPLAY_AIRV)
			{
				c  = PIXRGB(clamp_flt(fabsf(vx[y][x]), 0.0f, 8.0f),//vx adds red
					clamp_flt(pv[y][x], 0.0f, 8.0f),//pressure adds green
					clamp_flt(fabsf(vy[y][x]), 0.0f, 8.0f));//vy adds blue
			}
			else if (display_mode & DISPLAY_AIRH)
			{
				float ttemp = hv[y][x]+(-MIN_TEMP);
				int caddress = restrict_flt((int)( restrict_flt(ttemp, 0.0f, MAX_TEMP+(-MIN_TEMP)) / ((MAX_TEMP+(-MIN_TEMP))/1024) ) *3, 0.0f, (1024.0f*3)-3);
				c = PIXRGB((int)((unsigned char)color_data[caddress]*0.7f), (int)((unsigned char)color_data[caddress+1]*0.7f), (int)((unsigned char)color_data[caddress+2]*0.7f));
				//c  = PIXRGB(clamp_flt(fabsf(vx[y][x]), 0.0f, 8.0f),//vx adds red
				//	clamp_flt(hv[y][x], 0.0f, 1600.0f),//heat adds green
				//	clamp_flt(fabsf(vy[y][x]), 0.0f, 8.0f));//vy adds blue
			}
			else if (display_mode & DISPLAY_AIRC)
			{
				int r;
				int g;
				int b;
				// velocity adds grey
				r = clamp_flt(fabsf(vx[y][x]), 0.0f, 24.0f) + clamp_flt(fabsf(vy[y][x]), 0.0f, 20.0f);
				g = clamp_flt(fabsf(vx[y][x]), 0.0f, 20.0f) + clamp_flt(fabsf(vy[y][x]), 0.0f, 24.0f);
				b = clamp_flt(fabsf(vx[y][x]), 0.0f, 24.0f) + clamp_flt(fabsf(vy[y][x]), 0.0f, 20.0f);
				if (pv[y][x] > 0.0f)
				{
					r += clamp_flt(pv[y][x], 0.0f, 16.0f);//pressure adds red!
					if (r>255)
						r=255;
					if (g>255)
						g=255;
					if (b>255)
						b=255;
					c  = PIXRGB(r, g, b);
				}
				else
				{
					b += clamp_flt(-pv[y][x], 0.0f, 16.0f);//pressure adds blue!
					if (r>255)
						r=255;
					if (g>255)
						g=255;
					if (b>255)
						b=255;
					c  = PIXRGB(r, g, b);
				}
			}
			for (j=0; j<CELL; j++)//draws the colors
				for (i=0; i<CELL; i++)
					g->vid[(x*CELL+i) + (y*CELL+j)*(XRES+BARSIZE)] = c;
		}
#else
	int sdl_scale = 1;
	GLuint airProg;
	if(display_mode & DISPLAY_AIRC)
	{
		airProg = airProg_Cracker;
	}
	else if(display_mode & DISPLAY_AIRV)
	{
		airProg = airProg_Velocity;
	}
	else if(display_mode & DISPLAY_AIRP)
	{
		airProg = airProg_Pressure;
	}
	else
	{
		return;
	}

    glEnable( GL_TEXTURE_2D );
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, partsFbo);
    glTranslated(0, MENUSIZE, 0);

	glUseProgram(airProg);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, airVX);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, XRES/CELL, YRES/CELL, GL_RED, GL_FLOAT, sim->air->vx);
    glUniform1i(glGetUniformLocation(airProg, "airX"), 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, airVY);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, XRES/CELL, YRES/CELL, GL_GREEN, GL_FLOAT, sim->air->vy);
    glUniform1i(glGetUniformLocation(airProg, "airY"), 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, airPV);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, XRES/CELL, YRES/CELL, GL_BLUE, GL_FLOAT, sim->air->pv);
    glUniform1i(glGetUniformLocation(airProg, "airP"), 2);
    glActiveTexture(GL_TEXTURE0);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);
    glTexCoord2d(1, 1);
    glVertex3f(XRES*sdl_scale, YRES*sdl_scale, 1.0);
    glTexCoord2d(0, 1);
    glVertex3f(0, YRES*sdl_scale, 1.0);
    glTexCoord2d(0, 0);
    glVertex3f(0, 0, 1.0);
    glTexCoord2d(1, 0);
    glVertex3f(XRES*sdl_scale, 0, 1.0);
    glEnd();

    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glTranslated(0, -MENUSIZE, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glDisable( GL_TEXTURE_2D );
#endif
}

void Renderer::draw_grav_zones()
{
	int x, y, i, j;
	for (y=0; y<YRES/CELL; y++)
	{
		for (x=0; x<XRES/CELL; x++)
		{
			//if(sim->gravmask[y*(XRES/CELL)+x])
			{
				for (j=0; j<CELL; j++)//draws the colors
					for (i=0; i<CELL; i++)
						if(i == j)
							g->blendpixel(x*CELL+i, y*CELL+j, 255, 200, 0, 120);
						else
							g->blendpixel(x*CELL+i, y*CELL+j, 32, 32, 32, 120);
			}
		}
	}
}

void Renderer::drawblob(int x, int y, unsigned char cr, unsigned char cg, unsigned char cb)
{
	g->blendpixel(x+1, y, cr, cg, cb, 112);
	g->blendpixel(x-1, y, cr, cg, cb, 112);
	g->blendpixel(x, y+1, cr, cg, cb, 112);
	g->blendpixel(x, y-1, cr, cg, cb, 112);

	g->blendpixel(x+1, y-1, cr, cg, cb, 64);
	g->blendpixel(x-1, y-1, cr, cg, cb, 64);
	g->blendpixel(x+1, y+1, cr, cg, cb, 64);
	g->blendpixel(x-1, y+1, cr, cg, cb, 64);
}

Renderer::Renderer(Graphics * g, Simulation * sim):
	sim(NULL),
	g(NULL),
	zoomWindowPosition(0, 0),
	zoomScopePosition(0, 0),
	zoomScopeSize(10),
	ZFACTOR(8),
	zoomEnabled(false),
	decorations_enable(1)
{
	this->g = g;
	this->sim = sim;

	memset(fire_r, 0, sizeof(fire_r));
	memset(fire_g, 0, sizeof(fire_g));
	memset(fire_b, 0, sizeof(fire_b));

	//Set defauly display modes
	SetColourMode(COLOUR_DEFAULT);
	AddRenderMode(RENDER_BASC);
	AddRenderMode(RENDER_FIRE);

	//Prepare the graphics cache
	graphicscache = (gcache_item *)malloc(sizeof(gcache_item)*PT_NUM);
	memset(graphicscache, 0, sizeof(gcache_item)*PT_NUM);

	int fireColoursCount = 4;
	pixel fireColours[] = {PIXPACK(0xAF9F0F), PIXPACK(0xDFBF6F), PIXPACK(0x60300F), PIXPACK(0x000000)};
	float fireColoursPoints[] = {1.0f, 0.9f, 0.5f, 0.0f};

	int plasmaColoursCount = 5;
	pixel plasmaColours[] = {PIXPACK(0xAFFFFF), PIXPACK(0xAFFFFF), PIXPACK(0x301060), PIXPACK(0x301040), PIXPACK(0x000000)};
	float plasmaColoursPoints[] = {1.0f, 0.9f, 0.5f, 0.25, 0.0f};

	flm_data = Graphics::GenerateGradient(fireColours, fireColoursPoints, fireColoursCount, 200);
	plasma_data = Graphics::GenerateGradient(plasmaColours, plasmaColoursPoints, plasmaColoursCount, 200);

#ifdef OGLR
	//FBO Texture
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &partsFboTex);
	glBindTexture(GL_TEXTURE_2D, partsFboTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, XRES, YRES, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);

	//FBO
	glGenFramebuffers(1, &partsFbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, partsFbo);
	glEnable(GL_BLEND);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, partsFboTex, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // Reset framebuffer binding
	glDisable(GL_TEXTURE_2D);

	//Texture for air to be drawn
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &airBuf);
	glBindTexture(GL_TEXTURE_2D, airBuf);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, XRES/CELL, YRES/CELL, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	//Zoom texture
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &zoomTex);
	glBindTexture(GL_TEXTURE_2D, zoomTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	//Texture for velocity maps for gravity
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &partsTFX);
	glBindTexture(GL_TEXTURE_2D, partsTFX);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES/CELL, YRES/CELL, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);
	glGenTextures(1, &partsTFY);
	glBindTexture(GL_TEXTURE_2D, partsTFY);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES/CELL, YRES/CELL, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	//Texture for velocity maps for air
	//TODO: Combine all air maps into 3D array or structs
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &airVX);
	glBindTexture(GL_TEXTURE_2D, airVX);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES/CELL, YRES/CELL, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);
	glGenTextures(1, &airVY);
	glBindTexture(GL_TEXTURE_2D, airVY);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES/CELL, YRES/CELL, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);
	glGenTextures(1, &airPV);
	glBindTexture(GL_TEXTURE_2D, airPV);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES/CELL, YRES/CELL, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	//Fire alpha texture
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &fireAlpha);
	glBindTexture(GL_TEXTURE_2D, fireAlpha);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, CELL*3, CELL*3, 0, GL_ALPHA, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	//Glow alpha texture
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &glowAlpha);
	glBindTexture(GL_TEXTURE_2D, glowAlpha);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 11, 11, 0, GL_ALPHA, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);


	//Blur Alpha texture
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &blurAlpha);
	glBindTexture(GL_TEXTURE_2D, blurAlpha);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 7, 7, 0, GL_ALPHA, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	loadShaders();
#endif
	prepare_alpha(CELL, 1.0f);
}

void Renderer::CompileRenderMode()
{
	render_mode = 0;
	for(int i = 0; i < render_modes.size(); i++)
		render_mode |= render_modes[i];
}

void Renderer::AddRenderMode(unsigned int mode)
{
	for(int i = 0; i < render_modes.size(); i++)
	{
		if(render_modes[i] == mode)
		{
			return;
		}
	}
	render_modes.push_back(mode);
	CompileRenderMode();
}

void Renderer::RemoveRenderMode(unsigned int mode)
{
	for(int i = 0; i < render_modes.size(); i++)
	{
		if(render_modes[i] == mode)
		{
			render_modes.erase(render_modes.begin() + i);
			i = 0;
		}
	}
	CompileRenderMode();
}

void Renderer::SetRenderMode(std::vector<unsigned int> render)
{
	render_modes = render;
	CompileRenderMode();
}

std::vector<unsigned int> Renderer::GetRenderMode()
{
	return render_modes;
}

void Renderer::CompileDisplayMode()
{
	display_mode = 0;
	for(int i = 0; i < display_modes.size(); i++)
		display_mode |= display_modes[i];
}

void Renderer::AddDisplayMode(unsigned int mode)
{
	for(int i = 0; i < display_modes.size(); i++)
	{
		if(display_modes[i] == mode)
		{
			return;
		}
	}
	display_modes.push_back(mode);
	CompileDisplayMode();
}

void Renderer::RemoveDisplayMode(unsigned int mode)
{
	for(int i = 0; i < display_modes.size(); i++)
	{
		if(display_modes[i] == mode)
		{
			display_modes.erase(display_modes.begin() + i);
			i = 0;
		}
	}
	CompileDisplayMode();
}

void Renderer::SetDisplayMode(std::vector<unsigned int> display)
{
	display_modes = display;
	CompileDisplayMode();
}

std::vector<unsigned int> Renderer::GetDisplayMode()
{
	return display_modes;
}

void Renderer::SetColourMode(unsigned int mode)
{
	colour_mode = mode;
}

unsigned int Renderer::GetColourMode()
{
	return colour_mode;
}

Renderer::~Renderer()
{
	free(graphicscache);
	free(flm_data);
	free(plasma_data);
}
