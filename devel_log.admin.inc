<?php


function devel_log_log_view_filters($form, &$form_state) {
  $form = array();
  $sql =  'SELECT DISTINCT log_identifier FROM {devel_log_log}';
  $results = db_query($sql)->fetchCol();

  foreach ($results as $result) {
    $log_identifiers[$result] = $result;
  }

  asort($log_identifiers);


  $form['log_identifiers'] = array(
    '#type' => 'select',
    '#title' => 'Log Identifiers',
    '#options' => $log_identifiers,
    '#default_value' => isset($_SESSION['devel_log_log_identifiers']) ? $_SESSION['devel_log_log_identifiers'] : NULL,
    '#multiple' => TRUE,
  );

  $form['actions'] = array(
    '#type' => 'actions',
  );

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Apply'),
    '#submit' => array('devel_log_log_views_filters_submit'),
  );
  $form['actions']['reset'] = array(
    '#type' => 'submit',
    '#value' => 'Reset Filters',
    '#submit' => array('devel_log_log_views_filters_reset'),
  );

  return $form;
}


function devel_log_log_views_filters_submit(&$form, &$form_state) {
  $query = array();
  if (!empty($form_state['values']['log_identifiers'])) {
    $_SESSION['devel_log_log_identifiers'] = $form_state['values']['log_identifiers'];
  }
}


function devel_log_log_views_filters_reset(&$form, &$form_state) {
  unset($_SESSION['devel_log_log_identifiers']);
}


function devel_log_log_view() {
  $header = array('log_identifier', 'date', 'message', 'operations');
  $rows = array();

  $query = db_select('devel_log_log', 'dll')
  ->fields('dll', array('dllid', 'log_identifier', 'timestamp', 'message'))
  ->orderBy('dll.dllid', 'DESC')
  ->extend('PagerDefault')
  ->limit(30);

  if (!empty($_SESSION['devel_log_log_identifiers'])) {
    $query->condition('dll.log_identifier', $_SESSION['devel_log_log_identifiers'], 'IN');
  }

  $result = $query->execute();

  while ($log = $result->fetchAssoc()) {

    $data = NULL;
    if (isset($log['data'])) {
      $data = print_r(unserialize($log['data']), TRUE);
    }

    $rows[] = array(
      'log_identifier' => $log['log_identifier'],
      'date' => date('Y-m-d H:i', $log['timestamp']),
      'message' => $log['message'],
      'operations' => l('View', 'admin/reports/devel-log/' . $log['dllid']),
    );
  }

  $output = array();

  if (!empty($rows)) {
    $output[] = drupal_get_form('devel_log_log_view_filters');
    $output[] = array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows,
    );
    $output[] = array(
      '#theme' => 'pager'
    );
    $output[] = array(
      '#prefix' => '<div class="actions clearfix" style="padding:10px 0px;">',
      '#suffix' => '</div>',
      '#theme' => 'link',
      '#text' => 'Clear Log',
      '#path' => 'admin/reports/devel-log/clear',
      '#options' => array(
        'attributes' => array(
          'class' => array('button'),
          'style' => 'padding:5px 30px;font-weight:bold;',
        ),
        //REQUIRED:
        'html' => FALSE,
      ),
    );
  }
  else {
    $output[] = array(
      '#markup' => t('There are no records in the log.'),
    );
  }

  return $output;
}


function devel_log_log_record_view($dllid) {
  $log = db_query('SELECT * FROM {devel_log_log} WHERE dllid = :dllid', array(':dllid' => $dllid))->fetchAssoc();
//   dsm($log);
  $output = '';
  $output .= '<h3>Log Identifier:</h3>';
  $output .= '<div class="identifier">' . $log['log_identifier'] . '</div>';
  $output .= '<h3>Date:</h3>';
  $output .= '<div class="date">' . date('Y-m-d h:i', $log['timestamp']) . '</div>';
  $output .= '<h3>Message:</h3>';
  $output .= '<div class="message">' . $log['message'] . '</div>';
  if (isset($log['data'])) {
    $output .= '<h3>Data:</h3>';
    $output .= '<div class="data"><pre>' . print_r(unserialize($log['data']), TRUE) . '</pre></div>';
  }

  return $output;
}


function devel_log_log_clear_form($form, &$form_state) {

  $selected = isset($form_state['values']['clear']) ? $form_state['values']['clear'] : 'all';

  $form = array();
  $form['clear'] = array(
    '#type' => 'select',
    '#title' => 'Clear Which Records?',
    '#options' => array(
      'all' => t('All'),
      'log_id' => t('By Log Identifier'),
      'date' => t('By Date'),
      'combined_log_date' => t('By Log Identifier and Date'),
    ),
    '#ajax' => array(
      'callback' => 'devel_log_log_clear_form_ajax',
      'wrapper' => 'clear-options-wrapper',
    ),
  );

  $form['clear_options'] = array(
    '#type' => 'container',
    '#prefix' => '<div id="clear-options-wrapper">',
    '#suffix' => '</div>',
  );
  $query = db_query("SELECT DISTINCT log_identifier FROM {devel_log_log}");
  $log_ids = array();
  while ($log_id = $query->fetchField()) {
    $log_ids[$log_id] = $log_id;
  }
  if ($selected === 'log_id' || $selected === 'combined_log_date') {
    $form['clear_options']['log_identifiers'] = array(
      '#type' => 'select',
      '#multiple' => TRUE,
      '#title' => 'Log Identifiers',
      '#options' => $log_ids,
    );
  }

  if ($selected === 'date' || $selected === 'combined_log_date') {
    $form['clear_options']['date'] = array(
      '#type' => 'textfield',
      '#title' => 'Clear Records Before',
    );
  }


  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Clear Log Records',
  );

  $form['#submit'] = array('devel_log_log_clear_form_submit');

  return $form;
}

function devel_log_log_clear_form_ajax($form, $form_state) {
  return $form['clear_options'];
}


function devel_log_log_clear_form_submit(&$form, &$form_state) {
  $values = $form_state['values'];

  if ($values['clear'] === 'all') {
    $deleted = db_truncate('devel_log_log')->execute();
  }
  else {
    $query = db_delete('devel_log_log');
    if (isset($values['log_identifiers']) && is_array($values['log_identifiers'])) {
      $query->condition('log_identifier', $values['log_identifiers'], 'IN');
    }
    if (isset($values['date']) && $date = strtotime($values['date'])) {
      $query->condition('timestamp', $date, '<');
    }
    $deleted = $query->execute();
  }

  drupal_set_message(t('@records deleted from the Developer Log.', array('@records' => format_plural($deleted, '1 record', '@count records'))));
}


function devel_log_settings_form($form, &$form_state) {
  $form = array();

  $form['log_identifiers'] = array(
    '#type' => 'fieldset',
    '#title' => t('Log Identifiers'),
  );

  $log_identifiers = devel_log_get_log_identifiers();
  $options = array();
  $defaults = array();
  foreach ($log_identifiers as $log_identifier) {
    $options[$log_identifier->log_identifier] = $log_identifier->log_identifier;

    if ($log_identifier->logging_enabled) {
      $defaults[$log_identifier->log_identifier] = $log_identifier->log_identifier;
    }
  }

  $form['log_identifiers']['log_ids'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Enable Logging'),
    '#options' => $options,
    '#default_value' => $defaults,
  );

//   dsm($log_identifiers);
  $form['actions'] = array(
    '#type' => 'actions',
  );

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  $form['#submit'] = array('devel_log_settings_form_submit');

  return $form;
}


function devel_log_settings_form_submit(&$form, &$form_state) {
  $log_ids = $form_state['values']['log_ids'];

  $enabled_ids = array();
  $disabled_ids = array();
  foreach ($log_ids as $log_id => $enabled) {
    if ($enabled) {
      $enabled_ids[] = $log_id;
    }
    else {
      $disabled_ids[] = $log_id;
    }
  }

  if (!empty($enabled_ids)) {
    db_update('devel_log_identifiers')
    ->fields(array('logging_enabled' => 1))
    ->condition('log_identifier', $enabled_ids, 'IN')
    ->execute();
  }
  if (!empty($disabled_ids)) {
    db_update('devel_log_identifiers')
    ->fields(array('logging_enabled' => 0))
    ->condition('log_identifier', $disabled_ids, 'IN')
    ->execute();
  }

  drupal_set_message(t('The settings have been saved.'));
}