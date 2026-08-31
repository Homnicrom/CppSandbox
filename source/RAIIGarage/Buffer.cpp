#include <RAIIGarage/Buffer.h>
#include<print>

Buffer::Buffer() : m_Num{ new int{} }
{
  NotifyBufferOperation(BufferOperation::DefaultConstruct);
}

Buffer::Buffer(const int num) : m_Num{ new int{ num } }
{
  NotifyBufferOperation(BufferOperation::ValueConstruct);
}

Buffer::Buffer(const int num, IBufferObserver* const observer) : m_Num{ new int{ num } }, m_Observer{ observer }
{
  NotifyBufferOperation(BufferOperation::ValueConstruct);
}

Buffer::Buffer(const Buffer& buffer) : m_Num{ buffer.m_Num ? new int{ *buffer.m_Num } : nullptr }, m_Observer{ buffer.m_Observer }
{
  NotifyBufferOperation(BufferOperation::CopyConstruct);
}

Buffer::Buffer(Buffer&& buffer) noexcept : m_Num{ buffer.m_Num }, m_Observer{ buffer.m_Observer }
{
  buffer.m_Num = nullptr;

  NotifyBufferOperation(BufferOperation::MoveConstruct);
}

Buffer& Buffer::operator=(const Buffer& buffer)
{
  if (this != &buffer)
  {
    int* temp = buffer.m_Num ? new int(*buffer.m_Num) : nullptr; //For possible allocation failure, good pratice. Move assignment does not allocate
    delete m_Num;
    m_Num = temp;
  }

  NotifyBufferOperation(BufferOperation::CopyAssign);

  return *this;
}

Buffer& Buffer::operator=(Buffer&& buffer) noexcept
{
  if (this != &buffer)
  {
    delete m_Num;
    m_Num = buffer.m_Num;
    buffer.m_Num = nullptr;
  }

  NotifyBufferOperation(BufferOperation::MoveAssign);

  return *this;
}

Buffer::~Buffer()
{
  NotifyBufferOperation(BufferOperation::Destroy);
  delete m_Num;
}

std::optional<int> Buffer::GetValue() const
{
  if (m_Num)
  {
    return *m_Num;
  }

  return std::nullopt;
}

void Buffer::ReportBufferOperation(const BufferOperation operation) const
{
  const std::optional<int> value{ GetValue() };

  switch (operation)
  {
  case BufferOperation::DefaultConstruct: value ? std::println("Buffer constructor! {0}", *value) : std::println("Buffer constructor! Nullptr");
    break;

  case BufferOperation::ValueConstruct: value ? std::println("Buffer int argument constructor! {0}", *value) : std::println("Buffer int argument constructor! Nullptr");
    break;

  case BufferOperation::CopyConstruct: value ? std::println("Buffer COPY constructor! {0}", *value) : std::println("Buffer COPY constructor! Nullptr");
    break;

  case BufferOperation::MoveConstruct: value ? std::println("Buffer MOVE constructor! {0}", *value) : std::println("Buffer MOVE constructor! Nullptr");
    break;

  case BufferOperation::CopyAssign: value ? std::println("Buffer assignment! {0}", *value) : std::println("Buffer assignment! Nullptr");
    break;

  case BufferOperation::MoveAssign: value ? std::println("Buffer MOVE assignment! {0}", *value) : std::println("Buffer MOVE assignment! Nullptr");
    break;

  case BufferOperation::Destroy: value ? std::println("Buffer DELETE! {0}", *value) : std::println("Buffer DELETE! Nullptr");
    break;
  }
}

void Buffer::NotifyBufferOperation(const BufferOperation operation) const
{
  m_Observer ? m_Observer->OnBufferOperation(operation, GetValue()) : ReportBufferOperation(operation);
}