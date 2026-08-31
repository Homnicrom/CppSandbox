#pragma once
#include<optional>

class Buffer
{
public:
  enum class BufferOperation
  {
    DefaultConstruct,
    ValueConstruct,
    CopyConstruct,
    MoveConstruct,
    CopyAssign,
    MoveAssign,
    Destroy
  };

  class IBufferObserver
  {
  public:
    virtual void OnBufferOperation(BufferOperation operation, std::optional<int> value) = 0;

  protected:
    ~IBufferObserver() = default;
  };

  Buffer();
  explicit Buffer(int num);

  //Observer is not owning. The user must construct it before this Buffer and keeps it alive past it, as destructor notifies through it too
  Buffer(int num, IBufferObserver* observer);

  Buffer(const Buffer& buffer);
  Buffer(Buffer&& buffer) noexcept;

  Buffer& operator=(const Buffer& buffer);
  Buffer& operator=(Buffer&& buffer) noexcept;

  ~Buffer();

  std::optional<int> GetValue() const;

private:
  int* m_Num{};
  IBufferObserver* m_Observer{};

  //Default report
  void ReportBufferOperation(BufferOperation operation) const;

  void NotifyBufferOperation(BufferOperation operation) const;
};
