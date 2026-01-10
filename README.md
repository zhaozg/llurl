# llurl

llurl 是一个超高速 C 语言 URL 解析库，采用状态机与查表优化，兼容 http-parser API，专注于高性能、零依赖、易扩展和易测试。

## 主要特性

- **极致性能**：DFA 状态机 + 查表，单次解析可达 100M+ 次/秒
- **API 兼容**：兼容 http-parser 的 `http_parser_parse_url` 接口
- **完整支持**：支持所有 URL 组件、IPv6、CONNECT、特殊协议
- **零依赖**：纯 C99，无外部依赖
- **测试完善**：覆盖所有边界与异常情况

## 快速开始

### 构建

```bash
make                    # 构建静态/动态库
make test               # 运行测试
make run-benchmark      # 性能基准测试
```

### 基本用法

```c
#include "llurl.h"
#include <stdio.h>

int main() {
    const char *url = "http://example.com:8080/path?query=value#fragment";
    struct http_parser_url u;
    http_parser_url_init(&u);
    int result = http_parser_parse_url(url, strlen(url), 0, &u);
    if (result != 0) return 1;
    if (u.field_set & (1 << UF_HOST)) {
        printf("Host: %.*s\n", u.field_data[UF_HOST].len, url + u.field_data[UF_HOST].off);
    }
    if (u.field_set & (1 << UF_PORT)) {
        printf("Port: %u\n", u.port);
    }
    return 0;
}
```

更多用法与 API 详见 `llurl.h`。

## 性能与优化

- **查表优化**：统一 bitmask 查表，消除分支，提升 15%+ 性能
- **批量处理**：path/query/IPv6 等批量扫描，极大减少循环与分支
- **硬件加速**：充分利用 `memchr` 等 SIMD 优化
- **分支预测提示**：`LIKELY/UNLIKELY` 宏提升主路径性能
- **缓存友好**：所有查表数据小于 1KB，充分利用 L1 cache

详见 [OPTIMIZATION.md](doc/OPTIMIZATION.md) 和 [BENCHMARKS.md](doc/BENCHMARKS.md)。

## 测试

- **测试覆盖**：覆盖 50+ 种有效/无效/边界/协议场景
- **运行方式**：`make test`
- **测试用例说明**：详见 [TESTING.md](doc/TESTING.md)

## 设计与实现

- **DFA 状态机**：所有状态转移均查表实现，O(1) 跳转
- **统一查表**：所有字符分类、字段判断均用 bitmask 查表
- **批量处理**：path/query/IPv6 等场景采用批量扫描
- **API 设计**：所有结构体、函数均有详细注释，线程安全

## 贡献指南

1. Fork 仓库，创建 feature 分支
2. 开发/优化/补充文档，确保所有测试通过
3. 补充/更新相关文档与注释
4. 提交 Pull Request，描述变更点与优化点
5. 由维护者 review 并合并

## 参考资料

- [llhttp](https://github.com/nodejs/llhttp)
- [http-parser](https://github.com/nodejs/http-parser)
- RFC 3986
- 本项目 `doc/` 目录

## License

MIT License

---

详细优化、测试、性能报告请见 `doc/` 目录。

