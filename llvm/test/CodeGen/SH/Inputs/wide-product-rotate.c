typedef unsigned u32;
typedef unsigned long long u64;
#ifndef SHAPE
#define SHAPE 0
#endif
#ifndef RIGHT
#define RIGHT 0
#endif
#ifndef SITES
#define SITES 1
#endif
#define ROT(v,c) (RIGHT ? __builtin_rotateright32((v),(c)) : __builtin_rotateleft32((v),(c)))
__attribute__((noinline)) u64 bench_entry(u32 *data, u32 *aux, unsigned n, u32 seed) {
	u32 x=seed, y=~seed, a=1,b=3,c=5,d=7,e=11,f=13,g=17,h=19;
	for (unsigned i=0;i<n;++i) {
		u32 v=data[i], k=SHAPE==0 ? i+0x80000000u : aux[i];
		if (SHAPE==1) k=v;
		#pragma clang loop unroll(full)
		for (unsigned j=0;j<SITES;++j) {
			u32 q=v+(j*0x9e3779b9u), z=ROT(q,k+j);
			x^=z; y+=k+j;
			if (SHAPE==1) y+=q;
			if (SHAPE==2) { u64 p=(u64)q*(u64)(seed|0x80000000u); y+=(u32)p+(u32)(p>>32); x+=(x<z); }
			if (SHAPE==3) { a+=v; b^=k; c+=a^b; d+=c; e^=d; f+=e; g^=f; h+=g; y+=a+b+c+d+e+f+g+h; }
		}
	}
	return ((u64)x<<32)|y;
}
