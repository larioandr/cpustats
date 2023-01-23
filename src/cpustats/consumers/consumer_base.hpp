#ifndef CPUSTATS_CONSUMER_BASE_HPP
#define CPUSTATS_CONSUMER_BASE_HPP

class Consumer {
public:
    virtual ~Consumer() = default;

    virtual bool Start() = 0;
    virtual void BeginIter() = 0;
    virtual void EndIter() = 0;
    virtual void Finish() = 0;
};

#endif //CPUSTATS_CONSUMER_BASE_HPP
