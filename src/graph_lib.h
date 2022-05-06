#ifndef GRAPH_LIB_H
#define GRAPH_LIB_H

extern int* off_596E90;
extern int dword_596E94;
extern int dword_596E98;
extern int* off_596E9C;
extern int* off_596EA0;
extern unsigned char* off_596EA4;
extern int dword_596EA8;
extern int dword_596EAC;

int graphCompress(unsigned char* a1, unsigned char* a2, int a3);
void InitTree();
void InsertNode(int a1);
void DeleteNode(int a1);
int graphDecompress(unsigned char* a1, unsigned char* a2, int a3);

#endif /* GRAPH_LIB_H */
