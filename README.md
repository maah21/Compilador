# Desenvolvimento de um Compilador para a Linguagem C−

Apresentado à Universidade Federal de São Paulo como parte dos requisitos para aprovação na disciplina de **Compiladores**.  
**Docente:** Prof. Dr. Rodrigo Colnago Contreras

## Discentes

- **Ana Luiza Antonio Feitosa** (RA: 168517)  
- **Larissa Martins Sá** (RA: 168949)  
- **Marcella Fernandes Moraes** (RA: 170982)

---

## Resumo

Este repositório apresenta o desenvolvimento de um compilador para a linguagem **C−**, uma versão simplificada da linguagem C. O objetivo do projeto é aplicar os conceitos fundamentais estudados na disciplina de Compiladores, abordando as principais etapas do processo de compilação:

- **Análise léxica**
- **Análise sintática**
- **Construção da Árvore Sintática Abstrata (AST)**
- **Gerenciamento da tabela de símbolos**
- **Análise semântica**
- **Geração de código intermediário**

A linguagem **C−** foi escolhida por sua simplicidade, permitindo a implementação incremental dos módulos do compilador, conforme proposto por **Louden (2004)**. Este projeto visa consolidar o entendimento teórico dos algoritmos e das estruturas de dados envolvidas, além de proporcionar experiência prática no desenvolvimento de software de sistemas.

---

## Estrutura do Projeto

A estrutura do projeto é composta pelas seguintes etapas e módulos:

- **Analisador Léxico:** responsável pela identificação de tokens da linguagem.  
- **Analisador Sintático:** verifica a conformidade sintática dos tokens.  
- **Árvore Sintática Abstrata (AST):** representação hierárquica do programa.  
- **Tabela de Símbolos:** armazena as variáveis e funções definidas no código.  
- **Analisador Semântico:** verifica a consistência semântica do código.  
- **Geração de Código Intermediário:** produz uma representação intermediária do código.  

---

## Estrutura do Projeto

---

## Como Executar

### Pré-requisitos

- Compilador C/C++ compatível (exemplo: `gcc`, `g++`, `clang`)
- Sistema operacional Windows, Linux ou macOS

### Execução

#### 1) Instalar o Graphviz (para gerar a AST)

**Windows:**  
Instale o Graphviz e verifique se o comando `dot` funciona:
```bash
dot -V
```

**Linux (Debian/Ubuntu):**
```bash
sudo apt-get update
sudo apt-get install graphviz
dot -V
```

**Linux (Red Hat/CentOS/Fedora):**
```bash
sudo yum install graphviz
dot -V
```

**macOS (Homebrew):**
```bash
brew install graphviz
dot -V
```

#### 2) Clonar o repositório

No terminal, clone o repositório e entre na pasta do projeto:
```bash
git clone https://github.com/seuusuario/compilador-cminus.git
cd compilador-cminus
```

#### 4) Compilar o projeto

No terminal, navegue até a pasta do projeto e execute:
```bash
mingw32-make
```

ou
```bash
make clean
make
```

Isso irá gerar o executável `cminus` (ou `cminus.exe` no Windows).

#### 4) Executar o compilador (arquivo de teste .cm)

Após compilar, para rodar um arquivo `.cm`:
```bash
./cminus nome_do_arquivo_teste.cm
```

Exemplo (caso exista `test.cm` no projeto):
```bash
./cminus test.cm
```

#### 5) Gerar a imagem da AST (Graphviz)

Após executar o compilador, será gerado um arquivo `.dot` (ex.: `test.dot`). Converta o `.dot` para imagem `.png` com:
```bash
dot -Tpng test.dot -o test_ast.png
```

#### 6) Visualizar a imagem gerada

**Windows:**
```bash
start test_ast.png
```

**macOS:**
```bash
open test_ast.png
```

**Linux:**
```bash
xdg-open test_ast.png
```
