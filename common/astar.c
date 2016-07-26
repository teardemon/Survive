#include "astar.h"

int direction[8][2] = {
	{0,-1},//��
	{0,1},//��
	{-1,0},//��
	{1,0},//��
	{-1,-1},//����
	{1,-1},//����
	{-1,1},//����
	{1,1},//����
};	

static int8_t _less(struct heapele*_l,struct heapele*_r)
{
	AStarNode *l = (AStarNode*)((int8_t*)_l-sizeof(kn_dlist_node));
	AStarNode *r = (AStarNode*)((int8_t*)_r-sizeof(kn_dlist_node));
	return l->F < r->F;
}

static void _clear(struct heapele*e){
	AStarNode *n = (AStarNode*)((int8_t*)e-sizeof(kn_dlist_node));
	n->F = n->G = n->H = 0;
}


static inline AStarNode *get_node(AStar_t astar,int x,int y)
{
	if(x < 0 || x >= astar->xcount || y < 0 || y >= astar->ycount)
		return NULL;
	return &astar->map[y*astar->xcount+x];
}


AStar_t create_AStar(int xsize,int ysize,int *flags){
	AStar_t astar = calloc(1,sizeof(*astar)+sizeof(AStarNode)*xsize*ysize);
	astar->open_list = minheap_create(xsize*ysize,_less);
	kn_dlist_init(&astar->close_list);
	kn_dlist_init(&astar->neighbors);
	astar->xcount = xsize;
	astar->ycount = ysize;
	int i = 0;
	int j = 0;
	for( ; i < ysize; ++i)
	{
		for(j = 0; j < xsize;++j)
		{		
			AStarNode *tmp = &astar->map[i*xsize+j];
			tmp->x = j;
			tmp->y = i;
			tmp->block = flags[i*xsize+j];
		}
	}	
	return astar;
}


static inline void clear_neighbors(AStar_t astar){
	while(!kn_dlist_empty(&astar->neighbors))
		kn_dlist_pop(&astar->neighbors);
}


static inline kn_dlist* get_neighbors(AStar_t astar,AStarNode *node)
{
	clear_neighbors(astar);
	int32_t i = 0;
	for( ; i < 8; ++i)
	{
		int x = node->x + direction[i][0];
		int y = node->y + direction[i][1];
		AStarNode *tmp = get_node(astar,x,y);
		if(tmp){
			if(tmp->list_node.pre || tmp->list_node.next)
				continue;//��close����,������
			if(!tmp->block)
				kn_dlist_push(&astar->neighbors,(kn_dlist_node*)tmp);
		}
	}
	if(kn_dlist_empty(&astar->neighbors)) 
		return NULL;
	else 
		return &astar->neighbors;
}

//���㵽�����ٽڵ���Ҫ�Ĵ���
static inline float cost_2_neighbor(AStarNode *from,AStarNode *to)
{
	int delta_x = from->x - to->x;
	int delta_y = from->y - to->y;
	int i = 0;
	for( ; i < 8; ++i)
	{
		if(direction[i][0] == delta_x && direction[i][1] == delta_y)
			break;
	}
	if(i < 4)
		return 50.0f;
	else if(i < 8)
		return 60.0f;
	else
	{
		assert(0);
		return 0.0f;
	}	
}

static inline float cost_2_goal(AStarNode *from,AStarNode *to)
{
	int delta_x = abs(from->x - to->x);
	int delta_y = abs(from->y - to->y);
	return (delta_x * 50.0f) + (delta_y * 50.0f);
}

static inline void reset(AStar_t astar){
	//����close list
	AStarNode *n = NULL;
	while((n = (AStarNode*)kn_dlist_pop(&astar->close_list))){
		n->G = n->H = n->F = 0;
	}
	//����open list
	minheap_clear(astar->open_list,_clear);
}

int     isblock(AStar_t astar,int x,int y){
	AStarNode *n = get_node(astar,x,y);
	if(!n) return 1;
	else return n->block  ? 1 : 0;
}

int find_path(AStar_t astar,int x,int y,int x1,int y1,kn_dlist *path){
	AStarNode *from = get_node(astar,x,y);
	AStarNode *to = get_node(astar,x1,y1);
	if(!from || !to || from == to || to->block)		
		return 0;
	minheap_insert(astar->open_list,&from->heap);	
	AStarNode *current_node = NULL;	
	while(1){	
		struct heapele *e = minheap_popmin(astar->open_list);
		if(!e){ 
			reset(astar);
			return 0;
		}		
		current_node = (AStarNode*)((int8_t*)e-sizeof(kn_dlist_node));
		if(current_node == to){ 
			while(current_node)
			{
				kn_dlist_remove((kn_dlist_node*)current_node);//������close_list�У���Ҫɾ��
				if(current_node != from)//��ǰ��������뵽·������
					kn_dlist_push_front(path,(kn_dlist_node*)current_node);
				AStarNode *t = current_node;
				current_node = current_node->parent;
				t->parent = NULL;
				t->F = t->G = t->H = 0;
				t->heap.index = 0;
			}	
			reset(astar);
			return 1;
		}
		//current���뵽close��
		kn_dlist_push(&astar->close_list,(kn_dlist_node*)current_node);
		//��ȡcurrent�����ڽڵ�
		kn_dlist *neighbors =  get_neighbors(astar,current_node);
		if(neighbors)
		{
			AStarNode *n;
			while((n = (AStarNode*)kn_dlist_pop(neighbors))){
				if(n->heap.index)//��openlist��
				{
					float new_G = current_node->G + cost_2_neighbor(current_node,n);
					if(new_G < n->G)
					{
						//������ǰneighbor·������,����·��
						n->G = new_G;
						n->F = n->G + n->H;
						n->parent = current_node;
						minheap_change(astar->open_list,&n->heap);
					}
					continue;
				}
				n->parent = current_node;
				n->G = current_node->G + cost_2_neighbor(current_node,n);
				n->H = cost_2_goal(n,to);
				n->F = n->G + n->H;
				minheap_insert(astar->open_list,&n->heap);
			}
			neighbors = NULL;
		}	
	}	
}
