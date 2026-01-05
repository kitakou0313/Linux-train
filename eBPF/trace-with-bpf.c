#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <linux/sched.h>

// データの出力用の構造体を定義
struct event
{
    /* data */
    __u32 pid;
    __u32 tid;
    char comm[16];
    char filename[256];
};

// リングバッファを定義
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} events SEC(".maps");

// sys_enter_execvに対応したTracepointのコンテキストを表す構造体
struct trace_event_raw_sys_enter
{
    __u64 unused;
    long syscall_nr;
    unsigned long args[6];
};

// SECマクロによりBPFプログラムを実行するentry pointを指定
// HTMLのイベントリスナの登録と同じ感じ？
SEC("tp/syscalls/sys_enter_execve")
int trace_execve(struct trace_event_raw_sys_enter *ctx)
{
    struct  event *e;
    __u64 pid_tgid;

    // ring bufferの領域を確保
    e = bpf_ringbuf_reserve(
        &events,
        sizeof(*e),
        0
    );
    if (!e)
    {
        /* code */
        return 0;
    }

    // データを取得し、保存
    pid_tgid = bpf_get_current_pid_tgid();
    e->pid = pid_tgid >> 32;
    e->tid = pid_tgid;
    
    bpf_get_current_comm(&e->comm, sizeof(e->comm));

    // 実行ファイル名を取得
    bpf_probe_read_user_str(
        &e->filename,
        sizeof(e->filename),
        (const char *)ctx->args[0]
    );
    
    // ユーザー空間にデータを送信
    bpf_ringbuf_submit(e, 0);

    return 0;
}