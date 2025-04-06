#include <gtk/gtk.h>

#include <ext2fs/ext2fs.h>

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

typedef struct
{

    GtkTreeStore *store;

    GtkTreeIter *parent;

    int depth;

} DirIterContext; /// useing for tree

GtkWidget *tree_store_view, *inode_entry, *window;

ext2_filsys fs;

char *disk_image_path = "/dev/sdb"; 

// Show inode info in detail

void show_inode_info(ext2_ino_t ino)
{

    struct ext2_inode inode;

    char message[2048];

    ext2fs_read_inode_bitmap(fs);

    ext2fs_read_block_bitmap(fs);

    if (ext2fs_read_inode(fs, ino, &inode) != 0)
    {

        snprintf(message, sizeof(message), "Failed to read inode %u", ino);
    }
    else
    {

        int inode_allocated = ext2fs_test_inode_bitmap(fs->inode_map, ino);

        char blocks_info[1024] = "";

        for (int i = 0; i < EXT2_N_BLOCKS; i++)
        {

            if (inode.i_block[i])  // if 0 it already is empty so no need to print
            {

                int block_allocated = ext2fs_test_block_bitmap(fs->block_map, inode.i_block[i]);

                char buf[128];

                snprintf(buf, sizeof(buf), "Block[%d]: %u (allocated: %s)\n", i, inode.i_block[i], block_allocated ? "Yes" : "No");

                strcat(blocks_info, buf);
            }
        }

        snprintf(message, sizeof(message), "Inode: %u\nAllocated: %s\nMode: 0x%x\nSize: %u\nBlocks: %u\n%s", ino, inode_allocated ? "Yes" : "No", inode.i_mode, inode.i_size, inode.i_blocks, blocks_info);
    }

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", message);

    gtk_dialog_run(GTK_DIALOG(dialog));

    gtk_widget_destroy(dialog);
}

// Show superblock info

void show_superblock_info(GtkButton *button, gpointer user_data)
{

    char message[1024];

    snprintf(message, sizeof(message), "Inodes Count: %u\nBlocks Count: %u\nFree Blocks: %u\nFree Inodes: %u\nBlock Size: %u", fs->super->s_inodes_count, fs->super->s_blocks_count, fs->super->s_free_blocks_count, fs->super->s_free_inodes_count, EXT2_BLOCK_SIZE(fs->super));

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", message);

    gtk_dialog_run(GTK_DIALOG(dialog));

    gtk_widget_destroy(dialog);
}

// Recursive directory listing

void populate_tree(GtkTreeStore *store, GtkTreeIter *parent, ext2_ino_t dir_ino, int depth); /// declaraton to avoid errors

int dir_iter_cb(ext2_ino_t dir, int entry, struct ext2_dir_entry *dirent, int offset, int blocksize, char *buf, void *priv_data)
{

    DirIterContext *ctx = (DirIterContext *)priv_data;

    if (dirent->inode == 0 || dirent->name_len == 0 || dirent->name_len >= 255)

        return 0;

    char name[256] = {0};

    memcpy(name, dirent->name, dirent->name_len);

    name[dirent->name_len] = '\0';

    GtkTreeIter iter;

    gtk_tree_store_append(ctx->store, &iter, ctx->parent);

    gtk_tree_store_set(ctx->store, &iter, 0, name, 1, dirent->inode, -1);

    if (ctx->depth > 0)
    {

        struct ext2_inode child_inode;

        if (ext2fs_read_inode(fs, dirent->inode, &child_inode) == 0 && LINUX_S_ISDIR(child_inode.i_mode) && strcmp(name, ".") != 0 && strcmp(name, "..") != 0)
        {

            populate_tree(ctx->store, &iter, dirent->inode, ctx->depth - 1);
        }
    }

    return 0;
}

void populate_tree(GtkTreeStore *store, GtkTreeIter *parent, ext2_ino_t dir_ino, int depth)
{

    if (depth < 0)
        return;

    struct ext2_inode inode;

    if (ext2fs_read_inode(fs, dir_ino, &inode) != 0)

        return;

    if (!LINUX_S_ISDIR(inode.i_mode))

        return;

    DirIterContext ctx = {store, parent, depth};

    ext2fs_dir_iterate2(fs, dir_ino, 0, NULL, dir_iter_cb, &ctx);
}

void refresh_tree_from_inode(ext2_ino_t inode)
{

    GtkTreeStore *store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_UINT);

    GtkTreeIter root_iter;

    gtk_tree_store_append(store, &root_iter, NULL);

    gtk_tree_store_set(store, &root_iter, 0, "/", 1, inode, -1);

    populate_tree(store, &root_iter, inode, 2);

    gtk_tree_view_set_model(GTK_TREE_VIEW(tree_store_view), GTK_TREE_MODEL(store));
}

void on_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{

    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);

    GtkTreeIter iter;

    if (gtk_tree_model_get_iter(model, &iter, path))
    {

        guint inode;

        gtk_tree_model_get(model, &iter, 1, &inode, -1);

        show_inode_info(inode);
    }
}

void on_load_button_clicked(GtkButton *button, gpointer user_data)
{

    const gchar *text = gtk_entry_get_text(GTK_ENTRY(inode_entry));

    ext2_ino_t inode = atoi(text);

    refresh_tree_from_inode(inode);
}

void activate(GtkApplication *app, gpointer user_data)
{

    window = gtk_application_window_new(app);

    gtk_window_set_title(GTK_WINDOW(window), "Ext2 Debugfs GUI");

    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    gtk_container_add(GTK_CONTAINER(window), box);

    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    GtkWidget *superblock_button = gtk_button_new_with_label("Superblock Info");

    gtk_box_pack_start(GTK_BOX(sidebar), superblock_button, FALSE, FALSE, 0);

    inode_entry = gtk_entry_new();

    gtk_entry_set_placeholder_text(GTK_ENTRY(inode_entry), "Enter inode...");

    gtk_box_pack_start(GTK_BOX(sidebar), inode_entry, FALSE, FALSE, 0);

    GtkWidget *load_button = gtk_button_new_with_label("Load from inode");

    gtk_box_pack_start(GTK_BOX(sidebar), load_button, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), sidebar, FALSE, FALSE, 0);

    tree_store_view = gtk_tree_view_new();

    GtkTreeViewColumn *col_name = gtk_tree_view_column_new_with_attributes("Name", gtk_cell_renderer_text_new(), "text", 0, NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_store_view), col_name);

    GtkTreeViewColumn *col_inode = gtk_tree_view_column_new_with_attributes("Inode", gtk_cell_renderer_text_new(), "text", 1, NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_store_view), col_inode);

    gtk_box_pack_start(GTK_BOX(box), tree_store_view, TRUE, TRUE, 0);

    g_signal_connect(tree_store_view, "row-activated", G_CALLBACK(on_row_activated), NULL);

    g_signal_connect(load_button, "clicked", G_CALLBACK(on_load_button_clicked), NULL);

    g_signal_connect(superblock_button, "clicked", G_CALLBACK(show_superblock_info), NULL);

    gtk_widget_show_all(window);

    refresh_tree_from_inode(2);
}

int main(int argc, char **argv)
{

    errcode_t err = ext2fs_open(disk_image_path, EXT2_FLAG_RW, 0, 0, unix_io_manager, &fs);

    if (err)
    {

        fprintf(stderr, "Failed to open filesystem: %s\n", error_message(err));

        return 1;
    }

    GtkApplication *app = gtk_application_new("org.example.ext2gui", G_APPLICATION_FLAGS_NONE);

    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);

    ext2fs_close(fs);

    return status;
}
