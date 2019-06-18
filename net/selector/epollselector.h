#ifndef NET_EPOLL_SELECTOR_H_
#define NET_EPOLL_SELECTOR_H_

#include "selectordefs.h"

#include <vector>

#include "hidden_pointer_iterator.h"

struct epoll_event;

namespace net{
namespace selector{



class EPollSelector{
public:
	using iterator = hidden_pointer_iterator<epoll_event, FDResult>;

	EPollSelector(uint32_t maxFD);
	EPollSelector(EPollSelector &&other) /* = default */;
	EPollSelector &operator =(EPollSelector &&other) /* = default */;
	~EPollSelector() /* = default */;

	void swap(EPollSelector &other);

	bool insertFD(int fd, FDEvent event = FDEvent::READ);
	bool updateFD(int fd, FDEvent event);
	bool removeFD(int fd);

	WaitStatus wait(int timeout);

	iterator begin() const;
	iterator end() const;

private:
	int				epollFD_;
	std::vector<epoll_event>	fds_;
	int				fdsCount_	= 0;
	uint32_t			fdsConnected_	= 0;
};


} // namespace selector
} // namespace

#endif

