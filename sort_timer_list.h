#ifndef UTILS_SORT_TIMER_LIST
#define UTILS_SORT_TIMER_LIST

#include <time.h>
#define BUFFER_SIZE 64


class sort_timer_list
{
public:
	struct util_timer;

	struct client_data
	{
		sockaddr_in address;
		int sockfd;
		char buff[BUFFER_SIZE];
		util_timer* timer;
	};

	struct util_timer
	{
		time_t expire;
		void (*cb_func)(client_data* pdata);
		client_data* user_data;
		util_timer* prev;
		util_timer* next;
	};

public:
	sort_timer_list() : head(NULL), tail(NULL) {}
	~sort_timer_list();

	void add_timer(util_timer* timer);

	//	adjust a timer when ther timer node change
	void adjust_timer(util_timer* timer);

	void del_timer(util_timer* timer);

	void tick();
private:
	// insert timer behind node
	void add_timer(util_timer* timer, util_timer* node);
	
private:
	util_timer* head;
	util_timer* tail;
};

inline sort_timer_list::~sort_timer_list()
{
	util_timer* tmp = head;
	while(tmp)
	{
		head = tmp->next;
		detete tmp;
		tmp = head;
	}
}

void sort_timer_list::add_timer(util_timer* timer)
{
	if (!timer)
	{
		return;
	}

	if (!head)
	{
		head = tail = timer;
		return;
	}

	if (timer->expire < head->expire)
	{
		timer->next = head;
		head->prev = timer;
		head = timer;
		return;
	}

	add_timer(timer, head);
}

//	adjust a timer when ther timer node change
void sort_timer_list::adjust_timer(util_timer* timer)
{
	if (!timer)
	{
		return;
	}

	util_timer* tmp = timer->next;
	if (!tmp && tmp->expire >= timer->expire)
	{
		return;
	}

	if (head == timer)
	{
		head = head->next;
		head->prev = NULL;
		timer->next = NULL;
		add_timer(timer, head);
	}
	else
	{
		timer->prev->next = timer->next;
		timer->next->prev = timer->prev;
		add_timer(timer, timer->next);
	}
	
}


void sort_timer_list::del_timer(util_timer* timer)
{
	if (!timer)
	{
		return;
	}

	if (timer == head && timer == tail)
	{
		delete timer;
		head = NULL;
		tail = NULL;
		return;
	}

	if (timer == head)
	{
		head = timer->next;
		head->prev = NULL;
		delete timer;
		return;
	}

	if (timer == tail)
	{
		tail = timer->prev;
		tail->next = NULL;
		delete timer;
		return;
	}

	timer->prev->next = timer->next;
	timer->next->prev = timer->prev;
	delete timer;
	
}

void sort_timer_list::tick()
{
	if(!head)
	{
		return;
	}

	time_t cur = time(NULL);
	util_timer* tmp = head;
	while (tmp)
	{
		if (cur < tmp->expire)
		{
			break;
		}

		tmp->cb_func(tmp->user_data);
		head = tmp->next;
		if (head)
		{
			head->prev = NULL;
		}
		delete tmp;
		tmp = head;
	}
}
	
// insert timer behind node
void sort_timer_list::add_timer(util_timer* timer, util_timer* node)
{
	util_timer* prev = node;
	util_timer* tmp = prev->next;

	while (tmp)
	{
		if (timer->expire < tmp->expire)
		{
			prev->next = timer;
			timer->prev = prev;
			timer->next = tmp;
			tmp->prev = timer;
			break;
		}
		prev = tmp;
		tmp = tmp->next;
	}

	// add to the tail of tail
	if (!tmp)
	{
		prev->next = timer;
		timer->prev = prev;
		timer->next = NULL;
		tail = timer;
	}
}

#endif
