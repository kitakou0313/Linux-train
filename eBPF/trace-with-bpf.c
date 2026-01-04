#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <linux/sched.h>

// データの出力用の構造体を定義
struct event
{
    /* data */
    __u32 pid;
    __u32 ppid;
    char comm[16];
    char filename[256];
};

// リングバッファを定義
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF)
} events SEC(".maps");

// SECマクロによりBPFプログラムを実行するentry pointを指定
// HTMLのイベントリスナの登録と同じ感じ？
SEC("tp/syscalls/sys_enter_execve")
int trace_execve(struct trace_event_raw_sys_enter *ctx)
{
    struct  event *e;
    struct task_struct *task;

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
    
    // 現在のタスクの構造体を取得
    task = (struct task_struct *)bpf_get_current_task();

    // データを構造体に追加
    e->pid = bpf_get_current_pid_tgid() >> 32;
    e->ppid = BPF_CORE_READ(task, real_parent, tgid);
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