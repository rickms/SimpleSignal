/*
 *  SimpleSignals.h
 *  Signals
 *
 *  Created by Rick Smorawski on 5/1/15.
 *
 * Signals is an interpretation of Robert Penner's AS3 Signals, a variation of the Observer pattern, in C++ using
 * the latest language features of C++11 and C++14.  All memory allocations are done via smart pointers.  
 *
 * Usage : 
 *
 * Signal<int,int> onSomeEvent;
 *
 * // Lambda
 * onSomeEvent.add([](int a, int b) { cout << a << " + " << b << " = " << ( a + b ); });
 * // std::bind
 * onSomeEvent.add(std::bind(&ThisClass::callbackFunction,this,std::placeholders::_1,std::placeholders::_2));
 * // Still std::bind but hidden behind macro
 * onSomeEvent.add(SIGNAL_CB_METHOD2(ThisClass:callbackFunction));
 * // Dispatch
 * onSomeEvent.dispatch(1,2);
 *
 * ThisClass::callbackFunction(int a, int b) {
 *      cout << a << " + " << b << " = " << ( a + b );
 * }
 */

#ifndef Signal_
#define Signal_
#include <map>
#include <list>
#include <memory>

/*
 * Macros to be used when you want to use a method as a callback 
*/
#define SIGNAL_CB_METHOD(function) std::bind(&function,this)
#define SIGNAL_CB_METHOD1(function) std::bind(&function,this,std::placeholders::_1)
#define SIGNAL_CB_METHOD2(function) std::bind(&function,this,std::placeholders::_1,std::placeholders::_2)
#define SIGNAL_CB_METHOD3(function) std::bind(&function,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3)

using namespace std;
namespace signals {
    typedef int callback_id;            // each callback that is add()ed is assigned an id, used in removal of the callback
    
    /*
     * Signal
     *
     * This is your basic, unprioritized signal.  This uses c++11's varadic template to keep things simple and flexible
     */
    template <typename... callback_types>
    class Signal {
        // Some typedefs to maintain readablilty
        typedef std::function<void(callback_types...)> callback;    // uses std::function to support lambda's and std::bind
        typedef map<callback_id,callback> callback_map;
        
    private:
        int callbackId;                 // Should this be atomic?
        callback_map callbackMap;       // Map of callback_id's to callbacks
        
    public:
        
        Signal() : callbackId(0) {
        }
        
        callback_id add(callback cb) {
            auto newId = ++callbackId;
            callbackMap.insert(std::pair<callback_id,callback>(newId,cb));
            return newId;
        }
        
        void remove(callback_id cbId) {
            callbackMap.erase(cbId);
        }
        
        void removeAll() {
            callbackMap.clear();
        }
        
        void dispatch(callback_types... cb_types) {
            for(auto cb : callbackMap) {
                cb.second(cb_types...);
            }
        }
    };
    
    /*
     * PrioritizedCallback - Wrapper object for callback + priority + callbackId
     */
    template <typename... callback_types>
    class PrioritizedCallback {
        typedef std::function<void(callback_types...)> callback;
        
    private:
        callback priorityCallback;
        int priority;
        callback_id callbackId;
        
    public:
        PrioritizedCallback(callback callback, int priority, callback_id callbackId)
            : priorityCallback(callback),priority(priority),callbackId(callbackId){
        }
        
        callback getCallback() {
            return priorityCallback;
        }
        
        int getPriority() const {
            return priority;
        }
        
        int getCallbackId() {
            return callbackId;
        }
    };
    
    /*
     * Compare function to determine which PrioritizedCallback has the highest priority
     */
    template <typename... T> bool PriorityCompare(const PrioritizedCallback<T...> &a, const PrioritizedCallback<T...>  &b)
    {
        return a.getPriority() > b.getPriority();
    }
    
    /*
     * PrioritySignal
     *
     * Implementation of Prioritized Signal callbacks.  The callback add()'ed with the highest
     * priority will be called first.  Equal priorities are undefined, although they *should* be called
     * in the order they are added
     */
    template <typename... callback_types>
    class PrioritySignal
    {
        typedef void (*callback)(callback_types...); // typedef this to keep things clean
        
    private:
        int callbackId;
        list<PrioritizedCallback<callback_types...>> callbackList; // internal list of PrioritizedCallback's
        
    public:
        /*
         * Wrap the callback in a PrioritizeCallback object, and add it to the callbackList, then
         * sort the list.
         */
        callback_id add(callback cb, int priority = 0) {
            int newId = ++callbackId;
            callbackList.emplace_back(cb,priority,newId);
            callbackList.sort(PriorityCompare<callback_types...>);
            return newId;
        }
        
        /*
         * Remove a callback from the callbackList
         */
        void remove(callback_id callbackId) {
            for(auto i = callbackList.begin(); i != callbackList.end();) {
                if(i->getCallbackId() == callbackId) {
                    i = callbackList.erase(i);
                }  else {
                    ++i;
                }
            }
        }
        
        /*
         * Remove ALL callbacks from the callbackList
         */
        void removeAll() {
            callbackList.clear();
        }
        
        /*
         * Dispatch the signal, and any paramaters to all callbacks in the callbackList
         */
        void dispatch(callback_types... cb_types) {
            for(PrioritizedCallback<callback_types...> priorityCallback : callbackList) {
                priorityCallback.getCallback()(cb_types...);
            }
        }
    };
}
#endif
