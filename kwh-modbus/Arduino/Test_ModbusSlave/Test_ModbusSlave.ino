#include "Arduino.h"
#include "Modbus.h"
#include "ModbusArray.h"
#include "ModbusSerial.hpp"
#include "ModbusSlave.hpp"
#include "HardwareSerial.h"

  template<bool valid>
  struct enable_if;
  template <>
  struct enable_if<true> {
    using type = bool;
  };
  template <>
  struct enable_if<false>
  {};

constexpr bool GreaterThanZero(int N)
{
    return N > 0;
}

template <int, typename...>
struct Helper;

template <int N, typename Head, typename... Tail>
struct Helper<N, Head, Tail...>
{
    typedef typename Helper<N-1, Tail...>::type type;
};

template <typename Head, typename... Tail>
struct Helper<0, Head, Tail...>
{
    typedef Head& type;
};

template <int, typename...>
class TupleImpl;

template <>
class TupleImpl<-1>
{

};

template <typename Head>
class TupleImpl<0, Head>
{
protected:
    Head head;

public:
    template <int Depth>
    Head& get()
    {
        static_assert(Depth == 0, "Requested member deeper than Tuple");
        return head;
    }

    template <int Depth>
    const Head& get() const
    {
        static_assert(Depth == 0, "Requested member deeper than Tuple");
        return head;
    }
};

template <int N, typename Head, typename... Tail>
class TupleImpl<N, Head, Tail...>
{
protected:
    Head head;
    TupleImpl<N-1, Tail...> tail;



public:
    template <int M>
    typename enable_if<M == 0, Head&>::type get()
    {
        return head;
    }

    template <int M>
    typename enable_if<GreaterThanZero(M), typename Helper<M, Head, Tail...>::type>::type get()
    {
        return tail.get<M-1>();
    }

    template <int M>
    typename enable_if<M == 0, const Head&>::type get() const
    {
        return head;
    }

    template <int M>
    typename enable_if<GreaterThanZero(M), typename Helper<M, Head, Tail...>::type>::type get() const
    {
        return tail.get<M-1>();
    }
};

template <typename... Elements>
class Tuple : public TupleImpl<sizeof...(Elements)-1, Elements...>
{
public:
    static constexpr std::size_t size()
    {
        return sizeof...(Elements);
    }
};


class ArduinoFunctions
{
public:
  void delayMicroseconds(long len)
  {
    ::delayMicroseconds(len);
  }

  void digitalWrite(unsigned char pin, unsigned char value)
  {
    ::digitalWrite(pin, value);
  }

  uint8_t digitalRead(unsigned char pin)
  {
    return ::digitalRead(pin);
  }

  void pinMode(unsigned char pin, unsigned char mode)
  {
    ::pinMode(pin, mode);
  }
};

word *registers = new word[15];
ModbusSlave<HardwareSerial, ArduinoFunctions, ModbusArray> slave;

void setup() {
  // put your setup code here, to run once:
  //Serial *ser = &Serial1;

  int c = __COUNTER__;
  int l = __LINE__;

  for (int i = 0; i < 15; i++)
  {
    registers[i] = 0;
  }

  Serial.begin(9600);
  Serial.println("Starting...");
  
  slave.config(&Serial1, new ArduinoFunctions(), 19200, 4);
  Serial.println("Slave initialized");
  slave.init(registers, 0, 15, 30);
  slave.setSlaveId(3);
}

void loop() {
//  // put your main code here, to run repeatedly:
//  Serial.println(Serial1.available());12

  bool processed;
  bool broadcast;

  slave.task(processed, broadcast);

  for (int i = 0; i < 15; i++)
  {
    Serial.print(registers[i]);
    Serial.print(" ");
  }
  Serial.println("");
//  delay(2000);
//
//  if (Serial1.available())
//  {
//    while (Serial1.available())
//    Serial.print(Serial1.read());
//  }
//  if (Serial.available())
//  {
//    while (Serial.available())
//    {
//      digitalWrite(4, HIGH);
//      delay(30);
//      Serial1.write(Serial.read());
//      delay(30);
//      digitalWrite(4, LOW);
//      delay(30);
//    }
//  }
}
