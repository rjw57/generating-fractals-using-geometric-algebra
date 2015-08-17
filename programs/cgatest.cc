#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <libcga/cga.h>
#include <libcga/cga_stdfunc.h>

#define WIDTH 1280
#define HEIGHT 1024

#define EUCLIDEAN 0
  
#define MAXIT 128

#define USE_ALT_TRANS 1

#if EUCLIDEAN
#define MAP(z) cga_map(z)
#define IMAP(z) cga_imap(z)
#define TRANSLATOR(z, T) cga_translator(z, T)
#else
#define MAP(z) cga_maph(z)
#define IMAP(z) cga_imaph(z)
#define TRANSLATOR(z, T) cga_translatorh(z, T)
#endif

float euclidean_to_hyperbolic(float d)
{
  float s = sinh(d);
  return s / sqrt(1 + s*s);
}

float hyperbolic_to_euclidean(float d)
{
  return asinh(d / sqrt(1 - d*d));
}

void doOneIteration(float z[2], float c[2], float out[2])
{
	// The following performs a rotation about the origin of
	// \theta (where \theta is the angle between the vector and the
	// real axis) along with a dilation by |z|.
	out[0] = z[0]*z[0] - z[1]*z[1];
	out[1] = 2.f*z[0]*z[1];

	float magz = sqrt(z[0]*z[0] + z[1]*z[1]);
	float magcsq = c[0]*c[0] + c[1]*c[1];

#if !EUCLIDEAN
	if((magz >= 1.f) || (magcsq >= 1.f))
	{
		out[0] = z[0];
		out[1] = z[1];
		return;
	}
#endif

	// Undo the dilation.
	if(magz > 0.f)
	{
		out[0] /= magz; out[1] /= magz;

		// Now out[] contains *just* the rotation action.

		// Re-dilate by correct amount
		float dilationFactor = magz;

#if !EUCLIDEAN
		// If hyperbolic, calculate different dilation factor.
		float hypMag = hyperbolic_to_euclidean(magz);
		dilationFactor = euclidean_to_hyperbolic(hypMag * hypMag) / magz;
#endif

		out[0] *= dilationFactor; out[1] *= dilationFactor;
	}
	
	multiv_t Z, cvec, translator, dilator;
	Z = cga_new_multiv();
	cvec = cga_new_multiv();
	translator = cga_new_multiv();

#if USE_ALT_TRANS
	// Map Z = F(z)
	cga_assign_vector(Z, out, 2); 
	MAP(Z);

	// Form translator
	cga_assign_vector(cvec, c, 2);
	TRANSLATOR(cvec, translator);

	// apply the translator
	cga_geometric(translator, Z, Z);
	cga_reverse(translator, translator);
	cga_geometric(Z, translator, Z);
	
	// Reverse the mapping
	IMAP(Z);
#else
	// Map Z = F(c)
	cga_assign_vector(Z, c, 2); 
	MAP(Z);

	// Form translator
	cga_assign_vector(cvec, out, 2);
	TRANSLATOR(cvec, translator);

	// apply the translator
	cga_geometric(translator, Z, Z);
	cga_reverse(translator, translator);
	cga_geometric(Z, translator, Z);
	
	// Reverse the mapping
	IMAP(Z);
#endif

	out[0] = *( cga_get_element(Z, 1, 1) );
	out[1] = *( cga_get_element(Z, 1, 2) );

	cga_free_multiv(Z);
	cga_free_multiv(cvec);
	cga_free_multiv(translator);
}

float calculatePixel(float z[2], float c[2])
{
	const int maxIt = MAXIT;
	int itCount = 0;
	float tol = 4.f;

#if !EUCLIDEAN
	tol = euclidean_to_hyperbolic(2.f);
	tol *= tol;

	if((z[0]*z[0] + z[1]*z[1]) >= 1.f)
	{
		return 0.f;
	}

	if((c[0]*c[0] + c[1]*c[1]) >= 1.f)
	{
		return 0.f;
	}
#endif

	float magzsq = z[0]*z[0] + z[1]*z[1];
	while((itCount < maxIt) && (magzsq < tol))
	{
		float zout[2];

		doOneIteration(z, c, zout);
		z[0] = zout[0]; z[1] = zout[1];
		magzsq = z[0]*z[0] + z[1]*z[1];
		itCount++;
	}

	return (float)itCount / (float)maxIt;
}

void pixelValueToColour(float c, unsigned char* p)
{
	p[0] = p[1] = p[2] = 255;

	float b = (c / 2.f) + 0.5f;

	if((c < 0.f) || (c > 1.f))
		return;

#if 0
	if(c <= 0.25f) {
		p[0] = 0; p[1] = (unsigned char)(255.f*b*c*4.f); p[2] = (unsigned char)(255.f*b);
	} else if(c <= 0.5f) {
		p[0] = 0; p[1] = (unsigned char)(255.f*b); p[2] = (unsigned char)(255.f*b*(1.f-((c-0.25f)*4.f)));
	} else if(c <= 0.75f) {
		p[0] = (unsigned char)(255.f*b*((c-0.5f)*4.f)); p[1] = (unsigned char)(255.f*b); p[2] = 0;
	} else if(c <= 1.f) {
		p[0] = (unsigned char)(255.f*b); p[1] = (unsigned char)(255.f*b*(1.f-((c-0.75f)*4.f))); p[2] = 0;
	}
#else
	p[0] = p[1] = p[2] = (unsigned char) (255.f * (1.f-sqrt(c)));
#endif
}

int main(int argc, char **argv)
{
  char *outputfile;
  int outfd;
  unsigned char image[HEIGHT][WIDTH][3];
  
  if(argc == 2) {
    outputfile = argv[1];
  } else {
    fprintf(stderr, "synatax: %s <outputfile>\n", argv[0]);
    exit(1);
  }
  
  outfd = open(outputfile, O_WRONLY | O_CREAT);
  if(outfd == -1) {
    fprintf(stderr, "%s: Error opening '%s' for writing.", argv[0], 
                    outputfile);
    exit(2);
  }

  {
    char header[255];
    snprintf(header, 255, "P6\n%i %i 255\n", WIDTH, HEIGHT);
    write(outfd, header, strlen(header));
  }
  
  cga_init();
  
  float aspect = (float)(WIDTH) / (float)(HEIGHT);

  float z[2], c[2];

  for(int y=0; y<HEIGHT; y++) {
    printf("%i/%i\r", y+1, HEIGHT);
    fflush(stdout);
    for(int x=0; x<WIDTH; x++) {
      unsigned char *pixel = image[y][x];

#if 1
	  c[0] = 2.f * (float)(x - (WIDTH>>1)) / (float)(HEIGHT); //< To take account of aspect
	  c[1] = 2.f * (float)(y - (HEIGHT>>1)) / (float)(HEIGHT);
	  z[0] = 0.f;
	  z[1] = 0.f;
#else
	  z[0] = 2.f * (float)(x - (WIDTH>>1)) / (float)(HEIGHT); //< To take account of aspect
	  z[1] = 2.f * (float)(y - (HEIGHT>>1)) / (float)(HEIGHT);
	  c[0] = 0.35f;
	  c[1] = 0.203f;
#endif

      float pixVal = calculatePixel(z, c);
	  pixelValueToColour(pixVal, pixel);
    }
  }
  printf("\n");
  
  cga_finalise();
      
  write(outfd, image, WIDTH*HEIGHT*3);
  close(outfd);
  
  return 0;
}

// vim:sw=4:ts=4:cindent:noet
