#include <gtk/gtk.h>
#include <stdlib.h>
#include <ctype.h>

GtkWidget *window;
GtkWidget *start_screen;
GtkWidget *menu_screen;
GtkWidget *inode_screen;
GtkWidget *filepath_screen;
GtkWidget *inode_entry;
GtkWidget *filepath_entry;
GtkWidget *stack;

// Function to switch screens
void switch_screen(GtkWidget *screen) {
    gtk_widget_hide(start_screen);
    gtk_widget_hide(menu_screen);
    gtk_widget_hide(inode_screen);
    gtk_widget_hide(filepath_screen);
    gtk_widget_show_all(screen);
}

void show_error_popup(const gchar *message) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR,
                                               GTK_BUTTONS_CLOSE,
                                               "%s",
                                               message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Callback functions
void on_start_clicked(GtkWidget *widget, gpointer data) {
    gtk_stack_set_visible_child(GTK_STACK(stack), menu_screen);
}

void on_inode_clicked(GtkWidget *widget, gpointer data) {
    gtk_stack_set_visible_child(GTK_STACK(stack), inode_screen);
}

void on_filepath_clicked(GtkWidget *widget, gpointer data) {
    gtk_stack_set_visible_child(GTK_STACK(stack), filepath_screen);
}

void on_back_to_menu(GtkWidget *widget, gpointer data) {
    gtk_stack_set_visible_child(GTK_STACK(stack), menu_screen);
}

// Utility to validate integer input
gboolean is_valid_integer(const gchar *text) {
    if (*text == '\0') return FALSE;
    for (const gchar *p = text; *p; ++p) {
        if (!g_ascii_isdigit(*p)) return FALSE;
    }
    return TRUE;
}

void on_inode_entry_activate(GtkWidget *widget, gpointer data) {
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(inode_entry));
    if (is_valid_integer(text)) {
        g_print("Valid Inode Number: %d\n", atoi(text));
    } else {
        show_error_popup("Invalid Inode Number! Please enter an integer.");
    }
}

void on_filepath_entry_activate(GtkWidget *widget, gpointer data) {
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(filepath_entry));
    if (g_strcmp0(text, "") != 0) {
        g_print("File Path Entered: %s\n", text);
    } else {
        show_error_popup("File path cannot be empty!");
    }
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Main window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Debugfs Browser");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Start screen
    start_screen = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(start_screen, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(start_screen, GTK_ALIGN_CENTER);
    GtkWidget *start_button = gtk_button_new_with_label("Start Debugfs browser");
    gtk_box_pack_start(GTK_BOX(start_screen), start_button, FALSE, FALSE, 0);
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_clicked), NULL);

    // Menu screen
    menu_screen = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_halign(menu_screen, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(menu_screen, GTK_ALIGN_CENTER);
    GtkWidget *inode_button = gtk_button_new_with_label("Print Inode Data using Inode number");
    GtkWidget *filepath_button = gtk_button_new_with_label("Print File Data using File Path");
    gtk_box_pack_start(GTK_BOX(menu_screen), inode_button, FALSE, FALSE, 20);
    gtk_box_pack_start(GTK_BOX(menu_screen), filepath_button, FALSE, FALSE, 20);
    g_signal_connect(inode_button, "clicked", G_CALLBACK(on_inode_clicked), NULL);
    g_signal_connect(filepath_button, "clicked", G_CALLBACK(on_filepath_clicked), NULL);

    // Inode screen
    inode_screen = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(inode_screen, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(inode_screen, GTK_ALIGN_CENTER);
    GtkWidget *inode_label = gtk_label_new("Enter Inode Number");
    inode_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(inode_entry), "e.g., 12345");
    g_signal_connect(inode_entry, "activate", G_CALLBACK(on_inode_entry_activate), NULL);
    GtkWidget *back_button1 = gtk_button_new_with_label("Back");
    g_signal_connect(back_button1, "clicked", G_CALLBACK(on_back_to_menu), NULL);
    gtk_box_pack_start(GTK_BOX(inode_screen), inode_label, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(inode_screen), inode_entry, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(inode_screen), back_button1, FALSE, FALSE, 10);

    // Filepath screen
    filepath_screen = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(filepath_screen, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(filepath_screen, GTK_ALIGN_CENTER);
    GtkWidget *filepath_label = gtk_label_new("Enter File Path");
    filepath_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(filepath_entry), "/path/to/file");
    g_signal_connect(filepath_entry, "activate", G_CALLBACK(on_filepath_entry_activate), NULL);
    GtkWidget *back_button2 = gtk_button_new_with_label("Back");
    g_signal_connect(back_button2, "clicked", G_CALLBACK(on_back_to_menu), NULL);
    gtk_box_pack_start(GTK_BOX(filepath_screen), filepath_label, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(filepath_screen), filepath_entry, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(filepath_screen), back_button2, FALSE, FALSE, 10);

    // Main container
    stack = gtk_stack_new();
    gtk_stack_add_named(GTK_STACK(stack), start_screen, "start");
    gtk_stack_add_named(GTK_STACK(stack), menu_screen, "menu");
    gtk_stack_add_named(GTK_STACK(stack), inode_screen, "inode");
    gtk_stack_add_named(GTK_STACK(stack), filepath_screen, "filepath");
    gtk_container_add(GTK_CONTAINER(window), stack);

    // Initial screen
    gtk_stack_set_visible_child(GTK_STACK(stack), start_screen);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
