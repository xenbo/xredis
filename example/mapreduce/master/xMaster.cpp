#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#ifdef STD_STRING
#warning "STD STRING"
#include <string>
using std::string;
#else
#include <ext/vstring.h>
typedef __gnu_cxx::__sso_string string;
#endif

#include <unistd.h>

const size_t kMaxSize = 10 * 1000 * 1000;

class xSegment  // copyable
{
 public:
  string word;
  int64_t count = 0;

  explicit xSegment(std::istream* in)
    : in_(in)
  {
  }

  bool next()
  {
    string line;
    if (getline(*in_, line))
    {
      size_t tab = line.find('\t');
      if (tab != string::npos)
      {
        word = line.substr(0, tab);
        count = strtol(line.c_str() + tab, nullptr, 10);
        return true;
      }
    }
    return false;
  }

  bool operator<(const xSegment& rhs) const
  {
    return word > rhs.word;
  }

 private:
  std::istream* in_;
};

void output(int i, const std::unordered_map<string, int64_t>& counts)
{
  std::vector<std::pair<int64_t, string>> freq;
  for (const auto& entry : counts)
  {
    freq.push_back(make_pair(entry.second, entry.first));
  }
  std::sort(freq.begin(), freq.end());

  char buf[256];
  snprintf(buf, sizeof buf, "count-%05d", i);
  std::ofstream out(buf);
  std::cout << "  writing " << buf << std::endl;
  for (auto it = freq.rbegin(); it != freq.rend(); ++it)
  {
    out << it->first << '\t' << it->second << '\n';
  }
}

int combine(int argc, char* argv[])
{
  std::vector<std::unique_ptr<std::ifstream>> inputs;
  std::vector<xSegment> keys;

  for (int i = 1; i < argc; ++i)
  {
    char buf[256];
    snprintf(buf, sizeof buf, "segment-%05d", i);
    inputs.emplace_back(new std::ifstream(argv[i]));
    xSegment rec(inputs.back().get());
    if (rec.next())
    {
      keys.push_back(rec);
    }
    ::unlink(buf);
  }

  // std::cout << keys.size() << '\n';
  int m = 0;
  string last;
  std::unordered_map<string, int64_t> counts;
  std::make_heap(keys.begin(), keys.end());
  while (!keys.empty())
  {
    std::pop_heap(keys.begin(), keys.end());

    if (keys.back().word != last)
    {
      last = keys.back().word;
      if (counts.size() > kMaxSize)
      {
        std::cout << "    split" << std::endl;
        output(m++, counts);
        counts.clear();
      }
    }

    counts[keys.back().word] += keys.back().count;

    if (keys.back().next())
    {
      std::push_heap(keys.begin(), keys.end());
    }
    else
    {
      keys.pop_back();
    }
  }

  if (!counts.empty())
  {
    output(m++, counts);
  }
  std::cout << "combining done, " << m << std::endl;
  return m;
}

// ======= merge  =======

class xSource  // copyable
{
 public:
  explicit xSource(std::istream* in)
    : in(in)
  {
  }

  bool next()
  {
    string line;
    if (getline(*in, line))
    {
      size_t tab = line.find('\t');
      if (tab != string::npos)
      {
        count_ = strtol(line.c_str(), nullptr, 10);
        if (count > 0)
        {
          word = line.substr(tab+1);
          return true;
        }
      }
    }
    return false;
  }

  bool operator<(const xSource& rhs) const
  {
    return count < rhs.count;
  }

  void outputTo(std::ostream& out) const
  {
    out << count << '\t' << word << '\n';
  }

 private:
  std::istream* in;
  int64_t count = 0;
  string word;
};

void merge(int m)
{
  std::vector<std::unique_ptr<std::ifstream>> inputs;
  std::vector<xSource> keys;

  for (int i = 0; i < m; ++i)
  {
    char buf[256];
    snprintf(buf, sizeof buf, "count-%05d", i);
    inputs.emplace_back(new std::ifstream(buf));
    xSource rec(inputs.back().get());
    if (rec.next())
    {
      keys.push_back(rec);
    }
    ::unlink(buf);
  }

  std::ofstream out("output");
  std::make_heap(keys.begin(), keys.end());
  int topk = 10;
  while (!keys.empty())
  {
    if(--topk < 0)
    {
        break;
    }

    std::pop_heap(keys.begin(), keys.end());
    keys.back().outputTo(out);

    if (keys.back().next())
    {
      std::push_heap(keys.begin(), keys.end());
    }
    else
    {
      keys.pop_back();
    }
  }
  std::cout << "merging done\n";
}

int main(int argc, char* argv[])
{
  int m = combine(argc,argv);
  merge (m);
}
