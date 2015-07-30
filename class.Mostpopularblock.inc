<?php

/**
 * Class Mostpopular_block creates a block of HTML that can be displayed to the user
 * that contains all the most popular items available to Chartbeat
 */
class Mostpopular_block {
    protected $list_length;
    protected $api_key;
    protected $hostname;
    protected $debug_flag;
    protected $current_url;
    protected $json;
    protected $frontpage_title;

    /**
     * @return mixed
     */
    public function getFrontpageTitle()
    {
        return $this->frontpage_title;
    }

    /**
     * @param mixed $frontpage_title
     */
    public function setFrontpageTitle($frontpage_title)
    {
        $this->frontpage_title = $frontpage_title;
    }

    /**
     * @return mixed
     */
    public function getListLength()
    {
        return $this->list_length;
    }

    /**
     * @param mixed $list_length
     */
    public function setListLength($list_length)
    {
        $this->list_length = $list_length;
    }

    /**
     * @return mixed
     */
    public function getApiKey()
    {
        return $this->api_key;
    }

    /**
     * @param mixed $api_key
     */
    public function setApiKey($api_key)
    {
        $this->api_key = $api_key;
    }

    /**
     * @return mixed
     */
    public function getHostname()
    {
        return $this->hostname;
    }

    /**
     * @param mixed $hostname
     */
    public function setHostname($hostname)
    {
        $this->hostname = $hostname;
    }

    /**
     * @return mixed
     */
    public function getDebugFlag()
    {
        return $this->debug_flag;
    }

    /**
     * @param mixed $debug_flag
     */
    public function setDebugFlag($debug_flag)
    {
        $this->debug_flag = $debug_flag;
    }

    /**
     * @return mixed
     */
    public function getCurrentUrl()
    {
        return $this->current_url;
    }

    /**
     * @param mixed $current_url
     */
    public function setCurrentUrl($current_url)
    {
        $this->current_url = $current_url;
    }

    /**
     * @return mixed
     */
    public function getJson()
    {
        return $this->json;
    }

    /**
     * @param mixed $json
     */
    public function setJson($json)
    {
        $this->json = $json;
    }

    /**
     * this function figures out the context of the block and initializes public properties accordingly.
     * It also checks the Drupal vars to make sure that the block has been properly configured.
     */
    public function initialize() {
        $this->setApiKey(variable_get('chartbeat_mostpopular_apikey'));
        $this->setListLength(variable_get('chartbeat_mostpopular_list_length',3));
        $this->setHostname(variable_get('chartbeat_mostpopular_host'));
        $this->setDebugFlag(variable_get('chartbeat_mostpopular_debugging'));
        $this->setFrontpageTitle(variable_get('chartbeat_mostpopular_frontpage_title'));

        $current_url = (isset($_SERVER['HTTPS']) ? "https" : "http") . "://{$_SERVER['HTTP_HOST']}{". request_uri() . "}";

        $this->setCurrentUrl($current_url);

        if (strlen($this->getApiKey()) == 0 ) {
            watchdog("chartbeat_mostpopular","API Key too short -- not properly configured");
            trigger_error('API Key too short -- not properly configured');
        }

        if (strlen($this->getListLength()) == 0 ) {
            watchdog("chartbeat_mostpopular","Items list is too short -- not properly configured");
            trigger_error("Items list is too short -- not properly configured");
        }

        if (strlen($this->getHostname()) == 0 ) {
            watchdog("chartbeat_mostpopular_host","Host name is too short --  not properly configured");
            trigger_error("Host name is too short --  not properly configured");
        }
        // if the Front Page title is not set - it will use the hostname instead
        if (strlen($this->getFrontpageTitle()) == 0 ) {
            $this->setFrontpageTitle($this->getHostname());
        }
    }

    /**
     * uses block properties to get chartbeat data
     * @return mixed
     */
    public function retrieve_chartbeat_data() {
        // pull charts from Chartbeat
        $url = 'http://api.chartbeat.com/live/toppages/v3';
        $data = array(
            'apikey' => $this->getApiKey(),
            'host' =>  $this->getHostname(),
            'limit' => $this->getListLength(),
        );

        $full_url = url($url, array('query' => $data));

        $json=drupal_http_request($full_url);

        $jsonfeed=drupal_json_decode($json->data,true);

        if ($this->debug_flag) {
            watchdog('chartbeat_mostpopular', '<pre>' . $json->data . '</pre>');
        }

        $this->setJson($jsonfeed);
    }

    /**
     * this function creates the HTML for the Chartbeat - Most Popular block
     * @param $jsonfeed
     * @return string
     */
    public function display() {
        $jsonfeed = $this->getJson();

        echo "<pre>";
        print_r($jsonfeed);
        echo "</pre>";
        $output = "";

        /// start building the HTML divs for the block
        $output .= '<ul>';

        $nid=$this->get_current_node_id();

        $limit = count($jsonfeed['pages']);

        if ($this->getDebugFlag()) {

            watchdog('number of items in jsonfeed[pages]', '<pre>' . $limit  . '</pre>');
        }

        if ($limit==0) {
            $output='';
            return $output;
        }

        // go through the Chartbeat data and traverse the list based on user configuration of $list_length
        for ($i = 0; $i < $limit; $i++) {
            if ($i > $this->getListLength()+1) {
                return;
            }
            // now that you know the url path and page title
            $urlpath[$i] = $jsonfeed['pages'][$i]['path'];
            $pagetitle[$i] = $jsonfeed['pages'][$i]['title'];

            // extract the page title (and not the other section and site stuff)
            $exploded_title=explode("|",$pagetitle[$i]);

            // convert any possible HTML entities in the page title
            $current_title=html_entity_decode($exploded_title[0], ENT_QUOTES | ENT_IGNORE, "UTF-8");

            // build  campaign string for Google analytics
            // this string uses the current page URL as the source and the title of the link it's going to as the content
            $ga_string = '';

            //  $utm_source=$_SERVER[REQUEST_URI];
            $utm_medium="text-link";
            $utm_content = trim($current_title);
            $utm_campaign = "mostpopular";
            $ga_string ="?utm_medium=text-link&utm_content=".$utm_content."&utm_campaign=".$utm_campaign;

            // this is the url we will attach to every HREF in the link
            $proposed_url='http://www.' . $urlpath[$i].$ga_string;

            $item_title=$this->get_current_title($urlpath[$i]);

            // <li><a><img><h3 class="title"></h3></a></li>
            $style_prefix='<li>';
            $style_postfix='</li>';

            if ($item_title=='') {
                $item_title="Home";
            }

            // using the proposed URL do checks on it to build the correct link
            $title='<h3 class="title">' . $item_title.'</h3>';
            $href=l($title,$proposed_url,array('html'=>'true'));
            $output .= $style_prefix.$href.$style_postfix;

        }
        $output .= '</ul>';


        return $output;
    }

    /**
     * used only by display method so that it can get the proper node titles for items found in Chartbeat
     *
     * @param $url
     * @return array
     */
    protected function get_current_title($url) {
        // check to see if the URL passed by Chartbeat is only the home page URL
        $hostname = $this->getHostname() . "/";
        $url=str_replace($hostname,"",$url);
        // drupal_lookup_path doesn't know what to do with a front page URL so it just calls itself home
        if (strlen($url)==0) {
            $title=$this->getFrontpageTitle();
        } else {
            $drupal_path = drupal_parse_url($url);
            $url=$drupal_path['path'];
            $node_path = drupal_lookup_path("source", $url);
            print_r($node_path);
            $node = menu_get_object("node", 1, $node_path);
            $title = $node->title;
        }
        return $title;
    }

    /**
     * used only by display method so it can learn what the current node ID is
     * @return mixed
     */
    protected function get_current_node_id() {
        // get current node
        if ($node = menu_get_object()) {
            // Get the nid
            $nid = $node->nid;
        }
        return $nid;
    }
}

