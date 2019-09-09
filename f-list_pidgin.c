
#include "f-list_pidgin.h"
#include "f-list_report.h"

#include "gtkimhtml.h"
#include "gtkconv.h"
#include "gtkutils.h"
#include "pidginstock.h"


static void (*chat_add_users_orig)(PurpleConversation *conv, GList *cbuddies, gboolean new_arrivals) = NULL;
static void (*chat_update_user_orig)(PurpleConversation *conv, const char *user) = NULL;

//TODO: fix these up so they're only one function ...
static gboolean flist_channel_activate_real(const gchar *host, const gchar *path) {
    PurpleAccount *pa;
    PurpleConnection *pc;
    GHashTable *components;

    purple_debug_info(FLIST_DEBUG, "We are attempting to join a channel. Account: %s Channel: %s\n", host, path);
    pa = flist_deserialize_account(host);
    if(!pa) {
        purple_debug_warning(FLIST_DEBUG, "Attempt failed. The account is not found.");
        return FALSE;
    }

    pc = purple_account_get_connection(pa);
    if(!pc) {
        purple_debug_warning(FLIST_DEBUG, "Attempt failed. The account has no connection.");
        return FALSE;
    }
    if(purple_connection_get_state(pc) != PURPLE_CONNECTED) {
        purple_debug_warning(FLIST_DEBUG, "Attempt failed. The account is not online.");
        return FALSE;
    }

    components = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
    g_hash_table_insert(components, (gpointer) "channel", (gpointer) purple_url_decode(path));
    flist_join_channel(pc, components);
    g_hash_table_destroy(components);

    return TRUE;
}

static gboolean flist_channel_activate(GtkIMHtml *imhtml, GtkIMHtmlLink *link) {
    const gchar *url = gtk_imhtml_link_get_url(link);
    gchar *host, *path, *user, *password;
    int port;
    gboolean ret = FALSE;

    url += strlen("flistc://");
    purple_debug_info(FLIST_DEBUG, "FList channel URL activated: %s\n", url);

    if(!purple_url_parse(url, &host, &port, &path, &user, &password)) {
        purple_debug_warning(FLIST_DEBUG, "The FList channel URL did not parse.");
        return FALSE;
    }

    purple_debug_info(FLIST_DEBUG, "The FList channel URL is parsed. Host: %s Path: %s\n", host, path);

    if(host && path) {
        gchar *host_fixed = g_strdup(purple_url_decode(host));
        gchar *path_fixed = g_strdup(purple_url_decode(path));
        ret = flist_channel_activate_real(host_fixed, path_fixed);
        g_free(host_fixed);
        g_free(path_fixed);
    }

    if(host) g_free(host);
    if(path) g_free(path);
    if(user) g_free(user);
    if(password) g_free(password);

    return ret;
}

gboolean flist_staff_activate_real(const gchar *host, const gchar *path) {
    PurpleAccount *pa;
    PurpleConnection *pc;
    FListAccount *fla;

    purple_debug_info(FLIST_DEBUG, "We are attempting to send a staff confirmation. Account: %s Callid: %s\n", host, path);
    pa = flist_deserialize_account(host);
    if(!pa) {
        purple_debug_warning(FLIST_DEBUG, "Attempt failed. The account is not found.");
        return FALSE;
    }

    pc = purple_account_get_connection(pa);
    if(!pc) {
        purple_debug_warning(FLIST_DEBUG, "Attempt failed. The account has no connection.");
        return FALSE;
    }
    if(purple_connection_get_state(pc) != PURPLE_CONNECTED) {
        purple_debug_warning(FLIST_DEBUG, "Attempt failed. The account is not online.");
        return FALSE;
    }

    fla = purple_connection_get_protocol_data(pc);
    flist_send_sfc_confirm(fla, path);

    return TRUE;
}

static gboolean flist_staff_activate(GtkIMHtml *imhtml, GtkIMHtmlLink *link) {
    const gchar *url = gtk_imhtml_link_get_url(link);
    gchar *host, *path, *user, *password;
    int port;
    gboolean ret = FALSE;

    url += strlen("flistsfc://");
    purple_debug_info(FLIST_DEBUG, "FList staff URL activated: %s\n", url);

    if(!purple_url_parse(url, &host, &port, &path, &user, &password)) {
        purple_debug_warning(FLIST_DEBUG, "The FList staff URL did not parse.");
        return FALSE;
    }

    purple_debug_info(FLIST_DEBUG, "The FList staff URL is parsed. Host: %s Path: %s\n", host, path);

    if(host && path) {
        gchar *host_fixed = g_strdup(purple_url_decode(host));
        gchar *path_fixed = g_strdup(purple_url_decode(path));
        ret = flist_staff_activate_real(host_fixed, path_fixed);
        g_free(host_fixed);
        g_free(path_fixed);
    }

    return ret;
}

GtkWidget *get_account_icon(FListAccount *fla)
{
    PurpleStoredImage *account_image = purple_buddy_icons_find_account_icon(fla->pa);
    GtkWidget *icon = NULL;
    if (account_image)
    {
        GdkPixbuf *pixbuf = pidgin_pixbuf_from_imgstore(account_image);
        GdkPixbuf *scale = gdk_pixbuf_scale_simple(pixbuf, 22, 22, GDK_INTERP_BILINEAR);
        icon = gtk_image_new_from_pixbuf(scale);
        gtk_widget_set_sensitive(icon, TRUE);
        g_object_unref(pixbuf);
        g_object_unref(scale);

        return icon;
    }

    return NULL;
}

static void alert_staff_button_clicked_cb(GtkButton* button, gpointer func_data) {
    PurpleConversation *convo = (PurpleConversation*) func_data;
    FListAccount *fla = purple_connection_get_protocol_data(purple_conversation_get_gc(convo));
    g_return_if_fail(fla);

    FListReport *flr = flist_report_new(fla, convo, NULL, NULL);
    flist_report_display_ui(flr);
}

static void character_button_clicked_cb(GtkButton* button, gpointer func_data) {
    PurpleConversation *conv = (PurpleConversation*) func_data;
    PurpleConnection *pc = purple_conversation_get_gc(conv);

    // This can happen if we're clicking on the character button while being offline
    if (pc == NULL ) {
        return;
    }

    FListAccount *fla = purple_connection_get_protocol_data(pc);

    g_return_if_fail(fla);

    flist_get_profile(pc, fla->character);
}

static void flist_update_tag_color(GtkTextTag *tag, FListAccount *fla, const char *charname)
{
    GdkColor color;

    FListCharacter *character = flist_get_character(fla, charname);
    if (!character || !gdk_color_parse(flist_gender_color(character->gender), &color))
        return;

    g_object_set(G_OBJECT(tag), "foreground-set", TRUE, "foreground-gdk", &color, NULL);
}

static void flist_text_tag_table_tag_added_cb(GtkTextTagTable *text_tag_table, GtkTextTag *tag, PurpleConversation *conv) {
    static gboolean changing = FALSE;

    if (changing)
        return;

    if (!tag)
        return;

    FListAccount *fla = purple_connection_get_protocol_data(purple_conversation_get_gc(conv));
    g_return_if_fail(fla);
    gboolean is_im = (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM);

    const char *charname;

    if (strncmp(tag->name, "BUDDY ", 6) == 0)
        charname = tag->name + 6;
    else if (strcmp(tag->name, "send-name") == 0)
        charname = fla->character;
    else if (is_im && strcmp(tag->name, "receive-name") == 0)
        charname = purple_conversation_get_name(conv);
    else if (is_im && strcmp(tag->name, "action-name") == 0)
        charname = purple_conversation_get_name(conv);
    else
        return;

    changing = TRUE;
    flist_update_tag_color(tag, fla, charname);
    changing = FALSE;
}

static void flist_text_tag_table_tag_changed_cb(GtkTextTagTable *text_tag_table, GtkTextTag *tag, gboolean size_changed, PurpleConversation *conv) {
    flist_text_tag_table_tag_added_cb(text_tag_table, tag, conv);
}

static void flist_pidgin_conversation_created_cb(PurpleConversation *conv, FListAccount *fla)
{
    g_return_if_fail(PIDGIN_IS_PIDGIN_CONVERSATION(conv));
    PidginConversation *pidgin_conv;
    g_return_if_fail((pidgin_conv = PIDGIN_CONVERSATION(conv)));

    PurpleConnection *pc = purple_conversation_get_gc(conv);

    // Is this a conversation of our account?
    if (fla->pc != pc)
        return;

    // We are going to add a button to the conversation's toolbar that simply displays our characters name
    // This helps in case you are online with more than one character
    GtkWidget *toolbar = pidgin_conv->toolbar;

    // Check, if the button already exists
    GtkWidget *char_button = purple_conversation_get_data(conv, FLIST_CONV_ACCOUNT_BUTTON);
    GtkWidget *alert_button = purple_conversation_get_data(conv, FLIST_CONV_ALERT_STAFF_BUTTON);

    // Snippet taken from pidgin-otr 4.0.1
    /* Check if we've been removed from the toolbar; purple does this
     * when the user changes her prefs for the style of buttons to
     * display. */
    GList *children = gtk_container_get_children(GTK_CONTAINER(toolbar));
    if (char_button && !g_list_find(children, char_button)) {
        if (fla->show_own_character) {
            gtk_box_pack_end(GTK_BOX(toolbar), char_button, FALSE, FALSE, 0);
            gtk_widget_show_all(char_button);
        }
    }

    if (alert_button && !g_list_find(children, alert_button)) {
        gtk_box_pack_end(GTK_BOX(toolbar), alert_button, FALSE, FALSE, 0);
        gtk_widget_show_all(alert_button);
    }
    g_list_free(children);

    // Let's check if we have created the icon widget yet
    // In case Pidgin hasn't yet fetched the account's icon, we need to do it here
    if (char_button) {
        GtkWidget *icon = gtk_button_get_image(GTK_BUTTON(char_button));
        if (!icon) {
            icon = get_account_icon(fla);

            if (icon)
            {
                GList *children = gtk_container_get_children(GTK_CONTAINER(char_button));
                gtk_box_pack_start(GTK_BOX(g_list_first(children)->data), icon, TRUE, FALSE, 0);
            }
        }
    }

    if (fla->show_own_character) {
        // Everything we allocate (and add to the conversation UI) here gets destroyed automatically once the conversation is destroyed

        // Character button
        char_button = gtk_button_new_with_label(fla->character);
        GtkWidget *icon = get_account_icon(fla);
        if (icon)
            gtk_button_set_image(GTK_BUTTON(char_button), icon);

        gtk_button_set_relief(GTK_BUTTON(char_button), GTK_RELIEF_NONE);
        gtk_box_pack_end(GTK_BOX(toolbar), char_button, FALSE, FALSE, 0);

        gtk_widget_show_all(char_button);
        gtk_signal_connect(GTK_OBJECT(char_button), "clicked", GTK_SIGNAL_FUNC(character_button_clicked_cb), conv);
        purple_conversation_set_data(conv, FLIST_CONV_ACCOUNT_BUTTON, char_button);
    }

    // Alert Staff button
    alert_button = gtk_button_new_with_label("Alert Staff");
    gtk_button_set_image(GTK_BUTTON(alert_button), gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_SMALL_TOOLBAR));

    gtk_button_set_relief(GTK_BUTTON(alert_button), GTK_RELIEF_NONE);
    gtk_box_pack_end(GTK_BOX(toolbar), alert_button, FALSE, FALSE, 0);

    gtk_widget_show_all(alert_button);
    gtk_signal_connect(GTK_OBJECT(alert_button), "clicked", GTK_SIGNAL_FUNC(alert_staff_button_clicked_cb), conv);

    purple_conversation_set_data(conv, FLIST_CONV_ALERT_STAFF_BUTTON, alert_button);

    gboolean use_gender_colors = purple_account_get_bool(fla->pa, "use_gender_colors", TRUE);
    gboolean use_gender_colors_self = purple_account_get_bool(fla->pa, "use_gender_colors_self", TRUE);

    // Change nick color for our own name
    if (use_gender_colors && use_gender_colors_self) {
        GtkTextBuffer *buffer = GTK_IMHTML(pidgin_conv->imhtml)->text_buffer;

        GtkTextTag *buddy_tag = gtk_text_tag_table_lookup(
                                    gtk_text_buffer_get_tag_table(buffer), "send-name");

        if (!buddy_tag)
            return;

        flist_update_tag_color(buddy_tag, fla, fla->character);
    }

    // Change nick color for IM partners
    if (use_gender_colors) {
        GtkTextBuffer *buffer = GTK_IMHTML(pidgin_conv->imhtml)->text_buffer;

        if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM) {
            GtkTextTag *buddy_tag = gtk_text_tag_table_lookup(
                                        gtk_text_buffer_get_tag_table(buffer), "receive-name");

            const char *buddy_name = purple_conversation_get_name(conv);

            if (!buddy_tag || !buddy_name)
                return;

            flist_update_tag_color(buddy_tag, fla, buddy_name);
        }
        if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM) {
            GtkTextTag *buddy_tag = gtk_text_tag_table_lookup(
                                        gtk_text_buffer_get_tag_table(buffer), "action-name");

            const char *buddy_name = purple_conversation_get_name(conv);

            if (!buddy_tag || !buddy_name)
                return;

            flist_update_tag_color(buddy_tag, fla, buddy_name);
        }
        GtkTextTagTable *text_tag_table = gtk_text_buffer_get_tag_table(buffer);

        g_signal_connect_after(text_tag_table, "tag-added", (GCallback)flist_text_tag_table_tag_added_cb, conv);
        g_signal_connect_after(text_tag_table, "tag-changed", (GCallback)flist_text_tag_table_tag_changed_cb, conv);
    }
}

static void flist_pidgin_set_user_gender_color(PurpleConversation *conv, PurpleConvChatBuddy *buddy) {
    PurpleConnection *pc = purple_conversation_get_gc(conv);

    FListAccount *fla = purple_connection_get_protocol_data(pc);
    PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);
    PidginChatPane *gtkchat = gtkconv->u.chat;

    GtkTreeModel *tm = gtk_tree_view_get_model(GTK_TREE_VIEW(gtkchat->list));
    GtkListStore *ls = GTK_LIST_STORE(tm);

    GtkTreeIter iter;

    GtkTreePath *path;

    if (!buddy->ui_data || (path = gtk_tree_row_reference_get_path(buddy->ui_data)) == NULL)
        return;

    GdkColor color;
    FListCharacter *character = flist_get_character(fla, buddy->name);

    if (!character)
        return;

    if (!gdk_color_parse(flist_gender_color(character->gender), &color))
        return;

    if (gtk_tree_model_get_iter(tm, &iter, path)) {
        gtk_tree_path_free(path);
        gtk_list_store_set(ls, &iter, CHAT_USERS_COLOR_COLUMN, &color, -1); 
    }
}

/* The following callback is invoked when we are joining a channel and get the
 * user list or when new users join a channel
 *
 * So, what happens here is: A user joins the channel, libpurple will call
 * ui_ops->chat_add_users. This is normally handled by Pidgin, but we wrapped
 * it and call the original function in our wrapper.  As it runs, it sets
 * inserts a new row into the userlist's tree store, containing the color of
 * the name, given by get_nick_color() (@gtkconv.c).  When it returns, we take
 * over control again, find the row that just got created and replace the
 * original color with our desired one. Thankfully, Pidgin stores a (more or
 * less) direct pointer to the treestore item in buddy->ui_data.
 *
 * We implement a wrapper around gtkconv->ui_ops->chat_add_users instead of the
 * signal 'chat-buddy-joined', because the latter is called _before_ UI stuff
 * is set up and we won't be able to access a users entry in the room's tree
 * store.
 */
static void flist_pidgin_chat_add_users(PurpleConversation *conv, GList*cbuddies, gboolean new_arrivals) {
    chat_add_users_orig(conv, cbuddies, new_arrivals);

    PurpleConnection *pc = purple_conversation_get_gc(conv);

    // Is this a conversation of our plugin's account?
    if (!purple_strequal(
                purple_account_get_protocol_id(purple_connection_get_account(pc)),
                FLIST_PLUGIN_ID)) {
        return;
    }

    if (!purple_account_get_bool(purple_connection_get_account(pc),
                "use_gender_colors", TRUE)) {
        return;
    }

    GList *tmp = cbuddies;
    while (tmp != NULL) {
        PurpleConvChatBuddy *buddy = (PurpleConvChatBuddy*)tmp->data;
        flist_pidgin_set_user_gender_color(conv, buddy);

        tmp = tmp->next;
    }
}

/* The following callback is invoked whenever something about a user updates (e.g. its flags).
 * Pidgin handles those updates by removing the user's treestore row and reinserting a new one.
 * This means that we have to replace colors again.
 */
static void flist_pidgin_chat_update_user(PurpleConversation *conv, const char *user) {
    chat_update_user_orig(conv, user);

    PurpleConnection *pc = purple_conversation_get_gc(conv);

    // Is this a conversation of our plugin's account?
    if (!purple_strequal(
                purple_account_get_protocol_id(purple_connection_get_account(pc)),
                FLIST_PLUGIN_ID)) {
        return;
    }

    if (!purple_account_get_bool(purple_connection_get_account(pc),
                "use_gender_colors", TRUE)) {
        return;
    }

    PurpleConvChatBuddy *buddy = purple_conv_chat_cb_find(PURPLE_CONV_CHAT(conv), user);
    g_return_if_fail(buddy);

    flist_pidgin_set_user_gender_color(conv, buddy);
}

void flist_pidgin_init() {

    gtk_imhtml_class_register_protocol("flistc", flist_channel_activate, NULL);
    gtk_imhtml_class_register_protocol("flistsfc", flist_staff_activate, NULL);

    PurpleConversationUiOps *ui_ops = pidgin_conversations_get_conv_ui_ops();

    if (!chat_add_users_orig)
        chat_add_users_orig = ui_ops->chat_add_users;

    if (!chat_update_user_orig)
        chat_update_user_orig = ui_ops->chat_update_user;

    ui_ops->chat_add_users = flist_pidgin_chat_add_users;
    ui_ops->chat_update_user = flist_pidgin_chat_update_user;
}

void flist_pidgin_terminate() {
    gtk_imhtml_class_register_protocol("flistc", NULL, NULL);
    gtk_imhtml_class_register_protocol("flistsfc", NULL, NULL);

    PurpleConversationUiOps *ui_ops = pidgin_conversations_get_conv_ui_ops();
    ui_ops->chat_add_users = chat_add_users_orig;
    ui_ops->chat_update_user = chat_update_user_orig;
}

void flist_pidgin_enable_signals(FListAccount *fla)
{
    void *conv_handle = purple_conversations_get_handle();
    purple_signal_connect(conv_handle, "conversation-created", fla,
            PURPLE_CALLBACK(flist_pidgin_conversation_created_cb), fla);
}

void flist_pidgin_disable_signals(FListAccount *fla)
{
    void *conv_handle = purple_conversations_get_handle();
    purple_signal_disconnect(conv_handle, "conversation-created", fla,
            PURPLE_CALLBACK(flist_pidgin_conversation_created_cb));
}
