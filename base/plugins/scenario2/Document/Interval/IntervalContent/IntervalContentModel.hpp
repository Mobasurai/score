#pragma once
#include <QNamedObject>

class IntervalContentModel : public QNamedObject
{
	Q_OBJECT
	
	public:
	
		virtual ~IntervalContentModel() = default;
		int id() { return 0; }
		
	private:
	
};

